// $Id$
// 白凡

#ifndef _FRIIO_WHITE_H_
#define _FRIIO_WHITE_H_

#include "Recordable.hpp"
#include "AbstractFriio.hpp"

class FriioWhite : public AbstractFriio
{
public:
	FriioWhite();
	virtual ~FriioWhite() {
		if (initialized) {
			close();
		}
	};
	virtual void setChannel(BandType band, int channel);
	virtual const float getSignalLevel(void);
	
protected:
	/**
	 * 初期化処理(固定処理Aを除く)
	 * @param fd 対象ファイルディスクリプタ
	 */
	virtual void UsbInitalize(int fd);
	
	/**
	 * 初期化処理 : 処理開始
	 * 便宜上Beginにしているが、本来どんなことをしているかは不明
	 * @param fd 対象ファイルディスクリプタ
	 */
	virtual void UsbProcBegin(int fd);
	
	/**
	 * 処理終了
	 * 便宜上Endにしている。恐らくバッファクリア
	 * @param fd 対象ファイルディスクリプタ
	 */
	virtual void UsbProcEnd(int fd);
	
	/**
	 * ＬＥＤ制御
	 * @param fd 対象ファイルディスクリプタ
	 * @param color 色
	 */
	virtual void UsbProcLED(int fd, BonLedColor colorr, bool lnb_powered);
	
	/**
	 * 初期化処理 : 固定処理B
	 * @param fd 対象ファイルディスクリプタ
	 */
	virtual void UsbProcFixInitB(int fd);
};

#endif /* !defined(_FRIIO_WHITE_H_) */
