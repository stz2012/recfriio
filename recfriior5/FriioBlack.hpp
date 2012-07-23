// $Id$
// 黒凡

#ifndef _FRIIO_BLACK_H_
#define _FRIIO_BLACK_H_

#include "Recordable.hpp"
#include "AbstractFriio.hpp"

class FriioBlack : public AbstractFriio
{
public:
	FriioBlack();
	virtual ~FriioBlack() {
		if (initialized) {
			close();
		}
	};
	virtual void setChannel(BandType band, int channel);
	virtual const float getSignalLevel(void);
	
protected:
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

	/*
		bit 0   LNB 
		bit 1   STROBE
		bit 2   CLK
		bit 3   LED control
	*/

	#define FRIIO_PIC_LNB_0		(0)
	#define FRIIO_PIC_STROBE_0	(0)
	#define FRIIO_PIC_CLK_0		(0)
	#define FRIIO_PIC_LED_0		(0)

	#define FRIIO_PIC_LNB_1		(1 << 0)
	#define FRIIO_PIC_STROBE_1	(1 << 1)
	#define FRIIO_PIC_CLK_1		(1 << 2)
	#define FRIIO_PIC_LED_1		(1 << 3)

	virtual void UsbSendLED_bit(int fd, unsigned char data);
	virtual void UsbProcLED_full(int fd, int rgb_saturation, uint32_t color, unsigned char *);
};

#endif /* !defined(_FRIIO_BLACK_H_) */
