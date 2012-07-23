// $Id: AbstractFriio.cpp 10379 2012-02-16 17:51:49Z clworld $
// 白黒共通abstractクラス

#include <sys/file.h>
#include <sys/stat.h>
#include <errno.h>

#include <sstream>

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>

#include "usbops.hpp"
#include "AutoCloser.hpp"
#include "AbstractFriio.hpp"

/**
 * 受信開始
 */
void
AbstractFriio::startStream()
{
	assertInitialized();
	if (isStream) {
		return;
	}
	
	ringBuf = new RingBuf<AsyncRequest>(asyncBufSize);
	
	push_func = new PushIoThread(tunerFd, endpoint, requestReserveNum, requestPollingWait, ringBuf);
	pop_func  = new PopIoThread(tunerFd, ringBuf);
	
	push_thread = new boost::thread(boost::ref(*push_func));
	pop_thread  = new boost::thread(boost::ref(*pop_func));
	
	isStream = true;
}

/**
 * 受信停止
 */
void
AbstractFriio::stopStream()
{
	assertInitialized();
	if (!isStream) {
		return;
	}
	
	// 発行を止める。
	push_func->cancel();
	push_thread->join();
	
	// 発行された物を全て読み込む
	while (!ringBuf->isOverflow() && 0 < ringBuf->getWaitCount()) {
		const uint8_t *buf = NULL;
		try {
			getStream(&buf, 200);
		} catch (overflow_error& e) {
		} catch (usb_error& e) {
		}
	}
	
	pop_func->cancel();
	pop_thread->join();
	
	delete push_thread;
	delete pop_thread;
	
	delete push_func;
	delete pop_func;
	delete ringBuf;
	
	isStream = false;
}

/**
 * データを取得する
 * @param bufp データへのポインタを取得するポインタ(出力)
 *             タイムアウト時NULL
 * @param timeoutMsec データを待つ時間(msec)
 *                    0の場合タイムアウト無し
 * @return int 取得されたデータ長
 */
int
AbstractFriio::getStream(const uint8_t** bufp, int timeoutMsec)
{
	assertInitialized();
	if (!isStream) {
		startStream();
	}
	
	const AsyncRequest *req = ringBuf->getPopPtr(timeoutMsec);
	if (NULL == req) {
		*bufp = NULL;
		return 0;
	}
	
	if (req->urb.status != 0) {
		std::ostringstream stream;
		stream << "UrbStatus: " << req->urb.status;
		throw usb_error(stream.str());
	}
	
	int rlen = req->urb.actual_length;
	
	*bufp = req->buf;
	return rlen;
}

/**
 * データを取得可能か？
 * @return データが取得可能な場合true
 */
bool
AbstractFriio::isStreamReady()
{
	assertInitialized();
	if (!isStream) {
		startStream();
	}
	
	return ringBuf->getReadyCount() > 0;
}


/**
 * 初期化されていない場合exceptionを投げる。
 * @exception not_ready_error 初期化されていない
 */
void
AbstractFriio::assertInitialized() throw (not_ready_error)
{
	if (!initialized) {
		throw not_ready_error("not initialized.");
	}
}

/**
 * 白黒共通abstractクラス
 * Request=0x40 RequestNo=0x01 で Length=0 のベンダリクエストを送信
 * @param wValue wValue(USBのcontrolリクエストの値)
 * @param wIndex wIndex(USBのcontrolリクエストの値)
 */
void
AbstractFriio::UsbSendVendorRequest(int fd, uint16_t wValue, uint16_t wIndex)
{
	uint8_t buf[] = "";
	usbdevfs_ctrltransfer ctrl = {
		USB_DIR_OUT|USB_TYPE_VENDOR|USB_RECIP_DEVICE, 0x01, wValue, wIndex, 0, REQUEST_TIMEOUT, buf
	};
	
	usb_ctrl(fd, &ctrl);
}

/**
 * デバイスファイルがFriioの物かを確認する。
 */
bool
AbstractFriio::is_friio(const std::string &devfile)
{
	usb_device_descriptor usb_desc;
	try {
		usb_getdesc(devfile.c_str(), &usb_desc);
	} catch (usb_error &e) {
		if (log) *log << "usb_getdesc: " << e.what() << std::endl;
		return false;
	}
	
	if (TARGET_ID_VENDOR == usb_desc.idVendor && TARGET_ID_PRODUCT == usb_desc.idProduct) {
		return true;
	} else {
		return false;
	}
}

/**
 * Friioのデバイスファイルを検索する。
 */
std::vector<std::string>
AbstractFriio::searchFriios()
{
	boost::filesystem::path base_dir = boost::filesystem::path(BASE_DIR_UDEV);
	if (!boost::filesystem::exists(base_dir) || !boost::filesystem::is_directory(base_dir)) {
		base_dir = boost::filesystem::path(BASE_DIR_USBFS);
	}
	
	if (log) *log << "Search friios from dir: " << base_dir.string() << std::endl;
	
	std::vector<std::string> friios;
	// base_dirからfriioをデバイスファイルを探す。
	boost::filesystem::directory_iterator end;
	for (boost::filesystem::directory_iterator bus_iter(base_dir); bus_iter != end; ++bus_iter) {
		// バスでループ
		if (boost::filesystem::is_directory(*bus_iter)) {
			// ディレクトリのみ
			for (boost::filesystem::directory_iterator dev_iter(*bus_iter); dev_iter != end; ++dev_iter) {
				// バスに繋がってるデバイスでループ
				if (!boost::filesystem::is_directory(*dev_iter)) {
					// Friioであるかチェック
					if (is_friio(dev_iter->string())) {
						// ここまで来たらこのデバイスはfriioです。
						friios.push_back(dev_iter->string());
					}
				}
			}
		}
	}
	
	return friios;
}

/**
 * Friio認識ロックを取得する。
 * @param lockFile ロック用ファイル名
 * @return int ファイルディスクリプタ
 */
int
detectLock(std::string& lockFile) throw (io_error)
{
	// umaskを変更し、group,otherに書き込み権限を付加する。
	mode_t orgmask = umask(0);
	umask(orgmask & ~(S_IWGRP|S_IWOTH)); // go+w
	int fd = open(lockFile.c_str(), O_CREAT|O_TRUNC|O_WRONLY, S_IRWXU|S_IRWXG|S_IRWXO);
	int errno_bak = errno;
	// umaskを戻す。
	umask(orgmask);
	if (fd < 0) {
		// error
		std::ostringstream stream;
		stream << "failed to open lock file '" << lockFile << "' : " << stringError(errno_bak);
		throw io_error(stream.str());
	}
	
	AutoCloser autoClose(fd);
	
	int stat = flock(fd, LOCK_EX);
	if (stat < 0) {
		// error
		errno_bak = errno;
		std::ostringstream stream;
		stream << "failed to lock file '" << lockFile << "' : " << stringError(errno_bak);
		throw io_error(stream.str());
	}
	
	autoClose.cancel();
	
	return fd;
}

/**
 * チューナを開く
 * @param チューナが開けた場合true
 * @exception usb_error チューナ初期化時にエラーが発生した場合。
 */
const bool
AbstractFriio::open(bool lnb)
{
	if (initialized) {
		return true;
	}
	
	// Friio認識ロック取得
	int lockfd = detectLock(lockfile);
	AutoCloser lockClose(lockfd);
	
	// Friioの探索
	std::vector<std::string> friios = searchFriios();
	if (0 == friios.size()) {
		if (log) *log << "no friio found." << std::endl;
		return false;
	}
	if (log) *log << friios.size() << " friios found:" << std::endl;
	
	// 使用可能なFriioを探す
	int result_fd = -1;
	std::string result_devfile = "";
	for (std::vector<std::string>::iterator iter = friios.begin(); iter != friios.end(); ++iter) {
		std::string devfile = *iter;
		
		int retryCount = DETECT_ERROR_RETRY_MAX;
		while (0 < retryCount) {
			try {
				int fd = usb_open(devfile.c_str());
				AutoCloser autoClose(fd);
				if (-1 != result_fd) {
					// もう見つかっているのでclaimされているかのチェックだけ行う。
					std::string driver = usb_getdriver(fd, TARGET_INTERFACE);
					if (driver == "") {
						if (log) *log << devfile << ": not used." << std::endl;
					} else {
						if (log) *log << devfile << ": used by " << driver << std::endl;
					}
				} else {
					// まだ見つかってないのでclaimする。
					try {
						usb_claim(fd, TARGET_INTERFACE);
						usb_setinterface(fd, TARGET_INTERFACE, TARGET_ALTSETTING);
						
						// 種別判定
						TunerType type = UsbProcFixInitA(fd);
						if (targetType != type) {
							if (log) *log << devfile << " is " << tunerTypeName[type] << " skipped." << std::endl;
							break;
						}
						
						autoClose.cancel();
						result_fd = fd;
						result_devfile = devfile;
						if (log) *log << devfile << ": use this friio." << std::endl;
					} catch (usb_error &e) {
						retryCount--;
						if (log) *log << devfile << ": " << e.what() << std::endl;
						usleep(DETECT_ERROR_RETRY_INTERVAL * 1000);
						continue;
					}
				}
			} catch (busy_error& e) {
				if (log) *log << devfile << ": busy." << std::endl;
			} catch (usb_error& e) {
				if (log) *log << devfile << ": " << e.what() << std::endl;
			}
			
			break;
		}
	}
	
	if (-1 == result_fd) {
		if (log) *log << "no friio can be used." << std::endl;
		return false;
	}
	
	// UsbInitalize内でexceptionが発生した場合にclose()する。
	AutoCloser autoCloseUsbInit(result_fd);

	// 見つかった
	if (log) *log << "device: " << result_devfile << std::endl;
	tunerFd = result_fd;
	
	// 初期化する。
	UsbInitalize(tunerFd, lnb);
	
	autoCloseUsbInit.cancel();
	initialized = true;
	
	return true;
}

void
AbstractFriio::close()
{
	if (initialized) {
		AutoCloser autoClose(tunerFd);
		try {
			stopStream();
			UsbTerminate(tunerFd);
		} catch (usb_error &e) {
			if (log) *log << e.what() << " ignored." << std::endl;
		} catch (...) {
			if (log) *log << "AbstractFriio::close(): error. ignored." << std::endl;
		}
		initialized = false;
	}
}

/**
 * 固定処理A(デバイス判定)
 * @param fd 対象ファイルディスクリプタ
 * @return TunerType チューナの種別
 */
TunerType
AbstractFriio::UsbProcFixInitA(int fd)
{
	TunerType type;
	
	// STEP 1
	UsbSendVendorRequest(fd, 0x0002, 0x0011);
	// STEP 2
	UsbSendVendorRequest(fd, 0x0000, 0x0011);
	// STEP 3
	uint8_t buf1[] = "\xc1";
	usbdevfs_ctrltransfer ctrl1 = {
		USB_DIR_OUT|USB_TYPE_VENDOR|USB_RECIP_DEVICE, 0x03, 0x3000, 0x00fe, 1, REQUEST_TIMEOUT, buf1
	};
	usb_ctrl(fd, &ctrl1);
	
	// STEP 4
	uint8_t buf2[] = "";
	usbdevfs_ctrltransfer ctrl2 = {
		USB_DIR_IN |USB_TYPE_VENDOR|USB_RECIP_DEVICE, 0x02, 0x3000, 0x0100, 1, REQUEST_TIMEOUT, buf2
	};
	usb_ctrl(fd, &ctrl2);
	
	if (buf2[0]) {
		// 白凡と判定
		type = TUNER_FRIIO_WHITE;
	} else {
		// 黒凡と判定
		type = TUNER_FRIIO_BLACK;
		// STEP4 Append
		uint8_t buf3[] = "";
		usbdevfs_ctrltransfer ctrl3 = {
			USB_DIR_IN |USB_TYPE_VENDOR|USB_RECIP_DEVICE, 0x02, 0x3000, 0x0080, 1, REQUEST_TIMEOUT, buf3
		};
		usb_ctrl(fd, &ctrl3);
	}
	
	// STEP 5
	UsbSendVendorRequest(fd, 0x0004, 0x0030);
	
	return type;
}

/**
 * 初期化処理(固定処理Aを除く)
 * @param fd 対象ファイルディスクリプタ
 */
void
AbstractFriio::UsbInitalize(int fd, bool lnb)
{
	// 処理開始
	UsbProcBegin(fd);
	// ストリーム制御データ
	UsbProcStreamInit(fd);
	// ＬＥＤ制御(紫)
	UsbProcLED(fd, BON_LED_PURPLE, lnb);
	// 固定処理Ｂ
	UsbProcFixInitB(fd);
	// コントロールＬＥＤ制御(緑)
	UsbProcLED(fd, BON_LED_GREEN, lnb);
	// 初期化＼(＾o＾)／お疲れ様
}

/**
 * USB終了処理
 * @param fd 対象ファイルディスクリプタ
 */
void
AbstractFriio::UsbTerminate(int fd)
{
	UsbProcLED(fd, BON_LED_PURPLE, false);
	UsbProcEnd(fd);
}

/**
 * 処理終了
 * 便宜上Endにしている。恐らくバッファクリア
 * @param fd 対象ファイルディスクリプタ
 */
void
AbstractFriio::UsbProcEnd(int fd)
{
	UsbSendVendorRequest(fd, 0x0000, 0x0000);
}

/**
 * 初期化処理 : ストリーム制御データ
 * @param fd 対象ファイルディスクリプタ
 */
void
AbstractFriio::UsbProcStreamInit(int fd)
{
	UsbSendVendorRequest(fd, 0x0008, 0x0033);
	UsbSendVendorRequest(fd, 0x0040, 0x0037);
	UsbSendVendorRequest(fd, 0x001f, 0x003a);
	UsbSendVendorRequest(fd, 0x00ff, 0x003b);
	UsbSendVendorRequest(fd, 0x001f, 0x003c);
	UsbSendVendorRequest(fd, 0x00ff, 0x003d);
	UsbSendVendorRequest(fd, 0x0000, 0x0038);
	UsbSendVendorRequest(fd, 0x0000, 0x0035);
	UsbSendVendorRequest(fd, 0x0000, 0x0039);
	UsbSendVendorRequest(fd, 0x0000, 0x0036);
}
