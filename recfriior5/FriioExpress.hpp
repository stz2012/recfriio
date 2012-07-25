// $Id$
// 凡Exp

#ifndef _FRIIO_EXPRESS_H_
#define _FRIIO_EXPRESS_H_

#include "Recordable.hpp"
#include "AbstractFriio.hpp"

class FriioExpress : public AbstractFriio
{
public:
	FriioExpress();
	virtual ~FriioExpress() {
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
	virtual void UsbInitalize(int fd, bool lnb);
	
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
	virtual void UsbProcLED(int fd, BonLedColor color, bool lnb_powered);
	
	/**
	 * 初期化処理 : 固定処理B
	 * @param fd 対象ファイルディスクリプタ
	 */
	virtual void UsbProcFixInitB(int fd);
};

#endif /* !defined(_FRIIO_EXPRESS_H_) */
