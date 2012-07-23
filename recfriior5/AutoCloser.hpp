// $Id$
// ファイル自動close

#ifndef _AUTO_CLOSER_HPP_
#define _AUTO_CLOSER_HPP_

#include <stdio.h>

/**
 * ファイルを自動的にcloseする。
 */
class AutoCloser {
public:
	/**
	 * コンストラクタ
	 * @param fd 対象ファイルディスクリプタ
	 */
	AutoCloser(int fd) : fd(fd), doClose(true) {
	};
	
	/**
	 * デストラクタ
	 * ファイルを閉じる。
	 */
	virtual ~AutoCloser() {
		if (doClose) {
			close(fd);
		}
	}
	
	/**
	 * 指定したファイルを閉じないように指定する。
	 */
	virtual void cancel() {
		doClose = false;
	}
	
protected:
	/** ファイルディスクリプタ */
	int fd;
	/** ファイルを閉じるか？ */
	bool doClose;
};

#endif /* !defined(_AUTO_CLOSER_HPP_) */
