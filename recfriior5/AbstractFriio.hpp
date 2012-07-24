// $Id$
// 白黒共通クラス

#ifndef _ABSTRACT_FRIIO_H_
#define _ABSTRACT_FRIIO_H_

#include "setting.hpp"
#include "error.hpp"
#include "RingBuf.hpp"
#include "IoThread.hpp"
#include "Recordable.hpp"

// LED色
typedef enum {
	BON_LED_PURPLE, // 紫(待機中)
	BON_LED_GREEN,  // 緑(視聴中)
	BON_LED_RED,    // 赤(録画中)
	BON_LED_WHITE,  // 白(エラー、送信データサンプルがない＞、＜)
} BonLedColor;

class AbstractFriio : public Recordable
{
protected:
	AbstractFriio() :
		initialized(false),
		log(NULL),
		lockfile(DETECT_LOCKFILE_DEFAULT),
		tunerFd(-1),
		endpoint(TARGET_ENDPOINT),
		asyncBufSize(WHITE_ASYNCBUFFSIZE),
		requestReserveNum(WHITE_REQUEST_RESERVE_NUM),
		requestPollingWait(WHITE_REQUEST_POLLING_WAIT),
		isStream(false),
		ringBuf(NULL),
		push_func(NULL),
		pop_func(NULL),
		push_thread(NULL),
		pop_thread(NULL) {
	};
	
public:
	virtual ~AbstractFriio() {
		if (initialized) {
			close();
		}
	};
	
	virtual const bool open(bool lnb);
	
	virtual void close();
	
	virtual void setLog(std::ostream *plog) {
		log = plog;
	};
	
	virtual void setDetectLockFile(char *plockfile) {
		lockfile = std::string(plockfile);
	};
	
	virtual const TunerType getType() const {
		return targetType;
	};
	
	virtual const std::vector<BandType> getSupportBands() const {
		return supportBands;
	};
	
	virtual void setChannel(BandType band, int channel) = 0;
	virtual const float getSignalLevel(void) = 0;
	virtual void startStream();
	virtual void stopStream();
	virtual int getStream(const uint8_t** bufp, int timeoutMsec);
	virtual bool isStreamReady();
	
	virtual void clearBuffer() {
		assertInitialized();
		if (isStream) {
			stopStream();
			startStream();
		}
	};
	
protected:
	/** 初期化済? */
	bool initialized;
	
	/** ログ出力先 */
	std::ostream *log;
	
	/** ロックファイル名 */
	std::string lockfile;
	
	/** Friioのファイルディスクリプタ */
	int tunerFd;
	
	/** Friioの種別 */
	TunerType targetType;
	
	/** URB送信対象エンドポイント */
	int endpoint;
	
	/** ASYNCBUFFSIZE */
	unsigned long asyncBufSize;
	
	/** REQUEST_RESERVE_NUM */
	unsigned int requestReserveNum;
	
	/** REQUEST_POLLING_WAIT */
	unsigned int requestPollingWait;
	
	/** 使用可能な周波数帯域 */
	std::vector<BandType> supportBands;
	
	/** 読み出しスレッド実行中？ */
	bool isStream;
	
	/** 読み出しスレッドリングバッファ */
	RingBuf<AsyncRequest>* ringBuf;
	
	/** 読み出しスレッド 発行側 オブジェクト */
	PushIoThread* push_func;
	/** 読み出しスレッド 受信側 オブジェクト */
	PopIoThread*  pop_func;
	
	/** 読み出しスレッド 発行側 スレッド */
	boost::thread* push_thread;
	/** 読み出しスレッド 受信側 スレッド */
	boost::thread* pop_thread;
	
	/**
	 * デバイスファイルがFriioの物かを確認する。
	 * @param devfile デバイスファイル
	 * @return Friioである場合true;
	 */
#ifdef HDUS
	virtual bool is_friio(const std::string &devfile);
#else
	bool is_friio(const std::string &devfile);
#endif
	
	/**
	 * Friioのデバイスファイルを検索する。
	 * @return std::vector<std::string> Friioのデバイスファイル
	 */
	std::vector<std::string> searchFriios();
	
	/**
	 * 初期化されていない場合exceptionを投げる。
	 * @exception not_ready_error 初期化されていない
	 */
	void assertInitialized() throw (not_ready_error);
	
	/**
	 * Request=0x40 RequestNo=0x01 で Length=0 のベンダリクエストを送信
	 * @param fd 対象ファイルディスクリプタ
	 * @param wValue wValue(USBのcontrolリクエストの値)
	 * @param wIndex wIndex(USBのcontrolリクエストの値)
	 */
	void UsbSendVendorRequest(int fd, uint16_t wValue, uint16_t wIndex);
	
	/**
	 * 固定処理A(デバイス判定)
	 * @param fd 対象ファイルディスクリプタ
	 * @return TunerType チューナの種別
	 */
	virtual TunerType UsbProcFixInitA(int fd);
	
	/**
	 * 初期化処理(固定処理Aを除く)
	 * @param fd 対象ファイルディスクリプタ
	 */
	virtual void UsbInitalize(int fd, bool lnb);
	
	/**
	 * USB終了処理
	 * @param fd 対象ファイルディスクリプタ
	 */
	virtual void UsbTerminate(int fd);
	
	/**
	 * 初期化処理 : 処理開始
	 * 便宜上Beginにしているが、本来どんなことをしているかは不明
	 * @param fd 対象ファイルディスクリプタ
	 */
	virtual void UsbProcBegin(int fd) = 0;
	
	/**
	 * 処理終了
	 * 便宜上Endにしている。恐らくバッファクリア
	 * @param fd 対象ファイルディスクリプタ
	 */
	virtual void UsbProcEnd(int fd) = 0;
	
	/**
	 * 初期化処理 : ストリーム制御データ
	 * @param fd 対象ファイルディスクリプタ
	 */
	virtual void UsbProcStreamInit(int fd);
	
	/**
	 * ＬＥＤ制御
	 * @param fd 対象ファイルディスクリプタ
	 * @param color 色
	 */
	virtual void UsbProcLED(int fd, BonLedColor color, bool lnb) = 0;
	
	/**
	 * 初期化処理 : 固定処理B
	 * @param fd 対象ファイルディスクリプタ
	 */
	virtual void UsbProcFixInitB(int fd) = 0;
};

#endif /* !defined(_ABSTRACT_FRIIO_H_) */
