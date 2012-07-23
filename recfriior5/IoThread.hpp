// $Id$
// 非同期リクエスト発行スレッド
// PushIoThread, PopIoThread

#ifndef _IO_THREAD_HPP_
#define _IO_THREAD_HPP_

#include "setting.hpp"
#include "usbops.hpp"
#include "RingBuf.hpp"

/**
 * URB送信管理用
 */
struct AsyncRequest {
	usbdevfs_urb urb;
	int fd;
	uint8_t buf[TSDATASIZE];
};

/**
 * 非同期URBリクエスト発行スレッド
 */
class PushIoThread {
public:
	/**
	 * コンストラクタ
	 * @param fd USBのリクエストを送るファイルディスクリプタ
	 * @param endpoint USBのリクエストを送るエンドポイント
	 * @param requestReserveNum 非同期リクエスト予約数
	 * @param requestPollingWait 非同期リクエストポーリング間隔(ms)
	 * @param buf 非同期リクエストのデータを入れるリングバッファ
	 */
	PushIoThread(
		int fd,
		int endpoint,
		unsigned int requestReserveNum,
		unsigned int requestPollingWait,
		RingBuf<AsyncRequest> *buf)
	 : fd(fd),
	   endpoint(endpoint),
	   requestReserveNum(requestReserveNum),
	   requestPollingWait(requestPollingWait),
	   buf(buf),
	   is_cancelled(false) {
	}
	
	/** デストラクタ */
	virtual ~PushIoThread() {};
	
	/** スレッド処理本体 */
	void operator()();
	
	/** スレッドを停止させる */
	void cancel() {
		is_cancelled = true;
	}
	
protected:
	/** URB送信対象fd */
	int fd;
	/** URB送信対象エンドポイント */
	int endpoint;
	/** 非同期リクエスト予約数 */
	unsigned int requestReserveNum;
	/** 非同期リクエストポーリング間隔(ms) */
	unsigned int requestPollingWait;
	/** リングバッファ */
	RingBuf<AsyncRequest> *buf;
	/** 停止フラグon? */
	bool is_cancelled;
};

/**
 * 非同期URBリクエスト受信スレッド
 */
class PopIoThread {
public:
	/**
	 * コンストラクタ
	 * @param fd USBのリクエストを受け取るファイルディスクリプタ
	 * @param buf 非同期リクエストのデータを入れるリングバッファ
	 */
	PopIoThread(int fd, RingBuf<AsyncRequest> *buf);
	
	/** デストラクタ */
	virtual ~PopIoThread();
	
	/** スレッド処理本体 */
	void operator()();
	
	/** スレッドを停止させる */
	void cancel();
	
protected:
	/** usbfsから完了URBを読み込む */
	void reapUrbs();
	
	/** URB受信対象fd */
	int fd;
	/** リングバッファ */
	RingBuf<AsyncRequest> *buf;
	/** 停止フラグon? */
	bool is_cancelled;
	/** select中断用pipe(in) */
	int event_fd_push;
	/** select中断用pipe(out) */
	int event_fd_pop;
};

#endif /* !defined(_IO_THREAD_HPP_) */
