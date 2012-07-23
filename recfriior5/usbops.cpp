// $Id: usbops.cpp 10379 2012-02-16 17:51:49Z clworld $
// USB操作

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <sstream>

#include "usbops.hpp"
#include "AutoCloser.hpp"

/**
 * usb_device_descriptorを取得する。
 * @param devfile デバイスファイル
 * @param desc usb_device_descriptorへのポインタ(出力)
 * @exception usb_error 失敗時
 */
void
usb_getdesc(const char *devfile, usb_device_descriptor* desc) throw (usb_error)
{
	int f = open(devfile, O_RDONLY);
	if (-1 == f) {
		std::ostringstream stream;
		stream << "can't open usbdevfile to read '" << devfile << "'";
		throw usb_error(stream.str());
	}
	
	AutoCloser autoClose(f);
	
	memset(desc, '\0', sizeof(usb_device_descriptor));
	ssize_t rlen = read(f, desc, sizeof(usb_device_descriptor));
	if (-1 == rlen) {
		std::ostringstream stream;
		stream << "can't read usbdevfile '" << devfile << "'";
		throw usb_error(stream.str());
	}
}

/**
 * デバイスを開く。
 * @param devfile デバイスファイル
 * @return ファイルディスクリプタ
 * @exception usb_error 失敗時
 */
int
usb_open(const char *devfile) throw (usb_error)
{
	// open
	int fd = open(devfile, O_RDWR);
	if (-1 == fd) {
		int errno_bak = errno;
		std::ostringstream stream;
		stream << "usb open failed: " << stringError(errno_bak);
		throw usb_error(stream.str());
	}
	return fd;
}

/**
 * 使用中か確認する。
 * @param fd 対象ファイルディスクリプタ
 * @param interface 対象インターフェース
 * @return 使用しているドライバ名 未使用時""
 * @exception usb_error USBのエラー時
 */
std::string
usb_getdriver(int fd, int interface) throw (usb_error)
{
	usbdevfs_getdriver driver_info;
	memset(&driver_info, '\0', sizeof(usbdevfs_getdriver));
	driver_info.interface = interface;
	int r = ioctl(fd, USBDEVFS_GETDRIVER, &driver_info);
	if (r < 0) {
		int errno_bak = errno;
		
		if (errno_bak == ENODATA) { // 未使用の場合ここにくる。
			return std::string("");
		}
		
		// エラー
		std::ostringstream stream;
		stream << "getdriver failed: " << stringError(errno);
		throw usb_error(stream.str());
	}
	
	return std::string(driver_info.driver);
}

/**
 * setinterfaceする。
 * @param fd 対象ファイルディスクリプタ
 * @param interface 対象インターフェース
 * @exception usb_error USBのエラー時
 */
void
usb_setinterface(int fd, int interface, int altsetting) throw (usb_error)
{
	usbdevfs_setinterface interface_info;
	interface_info.interface  = interface;
	interface_info.altsetting = altsetting;
	int r = ioctl(fd, USBDEVFS_SETINTERFACE, &interface_info);
	if (r < 0) {
		int errno_bak = errno;
		// エラー
		std::ostringstream stream;
		stream << "usb setinterface failed: " << stringError(errno_bak);
		throw usb_error(stream.str());
	}
}

/**
 * claimする。
 * @param fd 対象ファイルディスクリプタ
 * @param interface 対象インターフェース
 * @exception usb_error USBのエラー時
 * @exception busy_error 使用中
 */
void
usb_claim(int fd, unsigned int interface) throw (busy_error, usb_error)
{
	int r = ioctl(fd, USBDEVFS_CLAIMINTERFACE, &interface);
	if (r < 0) {
		int errno_bak = errno;
		if (errno_bak == EBUSY) { // BUSY?
			throw busy_error("usb interface busy.");
		}
		
		// エラー
		std::ostringstream stream;
		stream << "usb claim failed: " << stringError(errno_bak);
		throw usb_error(stream.str());
	}
}

/**
 * releaseする。
 * @param fd 対象ファイルディスクリプタ
 * @param interface 対象インターフェース
 * @exception usb_error USBのエラー時
 */
void
usb_release(int fd, unsigned int interface) throw (usb_error)
{
	int r = ioctl(fd, USBDEVFS_RELEASEINTERFACE, &interface);
	if (r < 0) {
		int errno_bak = errno;
		// エラー
		std::ostringstream stream;
		stream << "usb release failed: " << stringError(errno_bak);
		throw usb_error(stream.str());
	}
}

/**
 * コントロールリクエストを送信する。
 * @param fd 対象ファイルディスクリプタ
 * @param ctrl コントロールリクエスト
 * @exception usb_error USBのエラー時
 */
int
usb_ctrl(int fd, usbdevfs_ctrltransfer *ctrl) throw (usb_error)
{
	int r = ioctl(fd, USBDEVFS_CONTROL, ctrl);
	if (r < 0) {
		int errno_bak = errno;
		// エラー
		std::ostringstream stream;
		stream << "usb ctrl failed: " << r << " " << stringError(errno_bak);
		throw usb_error(stream.str());
	}
	return r;
}

/**
 * URBを送信する。
 * @param fd 対象ファイルディスクリプタ
 * @param urbp 送信するURBへのポインタ
 * @return int 結果(ioctl)
 */
int
usb_submiturb(int fd, usbdevfs_urb* urbp)
{
	return ioctl(fd, USBDEVFS_SUBMITURB, urbp);
}

/**
 * 完了したURBを受信する。
 * @param fd 対象ファイルディスクリプタ
 * @param urbpp 受信したURBを入れるポインタのポインタ
 * @return int 結果(ioctl)
 */
int
usb_reapurb_ndelay(int fd, usbdevfs_urb** urbpp)
{
	return ioctl(fd, USBDEVFS_REAPURBNDELAY, (void *)urbpp);
}

/**
 * 送信したURBをキャンセルする。
 * エラー処理未実施。
 * @param fd 対象ファイルディスクリプタ
 * @param urbp キャンセルするURBへのポインタ
 * @return int 結果(ioctl)
 */
int
usb_discardurb(int fd, usbdevfs_urb* urbp)
{
	return ioctl(fd, USBDEVFS_DISCARDURB, urbp);
}
