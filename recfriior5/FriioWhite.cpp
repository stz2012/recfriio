// $Id: FriioWhite.cpp 5689 2008-09-18 19:48:59Z clworld $
// 白凡

#include <glib.h>
#include <inttypes.h>
#include <math.h>
#include <string.h>
#include "usbops.hpp"
#include "FriioWhite.hpp"

FriioWhite::FriioWhite()
	: AbstractFriio()
{
	supportBands.push_back(BAND_UHF);
	targetType         = TUNER_FRIIO_WHITE;
	asyncBufSize       = WHITE_ASYNCBUFFSIZE;
	requestReserveNum  = WHITE_REQUEST_RESERVE_NUM;
	requestPollingWait = WHITE_REQUEST_POLLING_WAIT;
}

/**
 * 周波数帯域/チャンネルを設定する。
 * @param newBand 周波数帯域 UHF/BS/CS
 * @param newChannel チャンネル
 */
void
FriioWhite::setChannel(BandType newBand, int newChannel)
{
	if (BAND_UHF != newBand || newChannel < 13 || newChannel > 62) {
		throw traceable_error("unknown channel."); // TODO: 適当なエラークラス
	}
	
	int fd = tunerFd;
	
	// チャンネルデータ計算
	const uint32_t wChCode = 0x0E7F + 0x002A * (uint32_t)(newChannel - 13);
	
	uint8_t sbuf[2][5];
	memset(sbuf, '\0', sizeof(sbuf));
	uint8_t rbuf[2][1];
	memset(rbuf, '\0', sizeof(rbuf));
	
	// 1回目リクエスト設定	
	sbuf[0][0] = 0xC0;
	sbuf[0][1] = (uint8_t)(wChCode >> 8);
	sbuf[0][2] = (uint8_t)(wChCode & 0xFF);
	sbuf[0][3] = 0xB2;
	sbuf[0][4] = 0x08;

	// 2回目リクエスト設定
	sbuf[1][0] = 0xC0;
	sbuf[1][1] = (uint8_t)(wChCode >> 8);
	sbuf[1][2] = (uint8_t)(wChCode & 0xFF);
	sbuf[1][3] = 0x9A;
	sbuf[1][4] = 0x50;
	
	// 1
	usbdevfs_ctrltransfer ctrl1 = {
		USB_DIR_OUT|USB_TYPE_VENDOR|USB_RECIP_DEVICE, 0x03, 0x3000, 0x00FE, sizeof(sbuf[0]), REQUEST_TIMEOUT, sbuf[0]
	};
	usb_ctrl(fd, &ctrl1);
	
	// 2
	usbdevfs_ctrltransfer ctrl2 = {
		USB_DIR_OUT|USB_TYPE_VENDOR|USB_RECIP_DEVICE, 0x03, 0x3000, 0x00FE, sizeof(sbuf[1]), REQUEST_TIMEOUT, sbuf[1]
	};
	usb_ctrl(fd, &ctrl2);
	
	// 3
	usbdevfs_ctrltransfer ctrl3 = {
		USB_DIR_IN|USB_TYPE_VENDOR|USB_RECIP_DEVICE, 0x02, 0x3000, 0x00B0, sizeof(rbuf[0]), REQUEST_TIMEOUT, rbuf[0]
	};
	usb_ctrl(fd, &ctrl3);
	
	// 4
	usbdevfs_ctrltransfer ctrl4 = {
		USB_DIR_IN|USB_TYPE_VENDOR|USB_RECIP_DEVICE, 0x02, 0x3000, 0x0080, sizeof(rbuf[1]), REQUEST_TIMEOUT, rbuf[1]
	};
	usb_ctrl(fd, &ctrl4);
}

/**
 * 信号レベルを取得する。
 * @return float 信号レベル
 */
const float
FriioWhite::getSignalLevel(void)
{
	int fd = tunerFd;
	
	float sig_level = 0;
	
	uint8_t sigbuf[37];
	uint32_t dwHex;
	memset(sigbuf, '\0', sizeof(sigbuf));
	usbdevfs_ctrltransfer ctrl = {
		USB_DIR_IN|USB_TYPE_VENDOR|USB_RECIP_DEVICE, 0x02, 0x3000, 0x0089, sizeof(sigbuf), REQUEST_TIMEOUT, sigbuf
	};
	usb_ctrl(fd, &ctrl);
	
	dwHex = ((uint32_t)(sigbuf[2]) << 16) | ((uint32_t)(sigbuf[3]) << 8) | (uint32_t)(sigbuf[4]);
	// 信号レベル計算
	if(!dwHex) {
		sig_level = 0.0f;
	} else {
		sig_level = (float)(68.4344993f - log10((double)dwHex) * 10.0 + pow(log10((double)dwHex) - 5.045f, (double)4.0f) / 4.09f);
	}
	
	return  sig_level;
}

/**
 * 初期化処理(固定処理Aを除く)
 * @param fd 対象ファイルディスクリプタ
 */
void
FriioWhite::UsbInitalize(int fd)
{
	// 処理開始
	UsbProcBegin(fd);
	// ストリーム制御データ
	UsbProcStreamInit(fd);
	// ＬＥＤ制御(紫)
	UsbProcLED(fd, BON_LED_PURPLE, false);
	// 固定処理Ｂ
	UsbProcFixInitB(fd);
	// コントロールＬＥＤ制御(緑)
	UsbProcLED(fd, BON_LED_GREEN, false);
	// 初期化＼(＾o＾)／お疲れ様
}

/**
 * 初期化処理 : 処理開始
 * 便宜上Beginにしているが、本来どんなことをしているかは不明
 * @param fd 対象ファイルディスクリプタ
 */
void
FriioWhite::UsbProcBegin(int fd)
{
	// 未使用
	UsbSendVendorRequest(fd, 0x0001, 0x0000);
	UsbSendVendorRequest(fd, 0x000f, 0x0006);
}

/**
 * 処理終了
 * 便宜上Endにしている。恐らくバッファクリア
 * @param fd 対象ファイルディスクリプタ
 */
void
FriioWhite::UsbProcEnd(int fd)
{
	UsbSendVendorRequest(fd, 0x0001, 0x0000);
}

/**
 * ＬＥＤ制御
 * @param fd 対象ファイルディスクリプタ
 * @param color 色
 */
void
FriioWhite::UsbProcLED(int fd, BonLedColor color, bool lnb_powered)
{
	switch(color)
	{
	case BON_LED_PURPLE:
		{
			UsbSendVendorRequest(fd, 0x000b, 0x0000);
			UsbSendVendorRequest(fd, 0x000f, 0x0000);
			UsbSendVendorRequest(fd, 0x0003, 0x0000);
			UsbSendVendorRequest(fd, 0x0007, 0x0000);
			UsbSendVendorRequest(fd, 0x000b, 0x0000);
			UsbSendVendorRequest(fd, 0x000f, 0x0000);
			UsbSendVendorRequest(fd, 0x0003, 0x0000);
			UsbSendVendorRequest(fd, 0x0007, 0x0000);
			UsbSendVendorRequest(fd, 0x0003, 0x0000);
			UsbSendVendorRequest(fd, 0x0007, 0x0000);
			UsbSendVendorRequest(fd, 0x000b, 0x0000);
			UsbSendVendorRequest(fd, 0x000f, 0x0000);
			UsbSendVendorRequest(fd, 0x0003, 0x0000);
			UsbSendVendorRequest(fd, 0x0007, 0x0000);
			UsbSendVendorRequest(fd, 0x000b, 0x0000);
			UsbSendVendorRequest(fd, 0x000f, 0x0000);
			UsbSendVendorRequest(fd, 0x000b, 0x0000);
			UsbSendVendorRequest(fd, 0x000f, 0x0000);
			UsbSendVendorRequest(fd, 0x0003, 0x0000);
			UsbSendVendorRequest(fd, 0x0007, 0x0000);
			UsbSendVendorRequest(fd, 0x000b, 0x0000);
			UsbSendVendorRequest(fd, 0x000f, 0x0000);
			UsbSendVendorRequest(fd, 0x000b, 0x0000);
			UsbSendVendorRequest(fd, 0x000f, 0x0000);
			UsbSendVendorRequest(fd, 0x000b, 0x0000);
			UsbSendVendorRequest(fd, 0x000f, 0x0000);
			UsbSendVendorRequest(fd, 0x000b, 0x0000);
			UsbSendVendorRequest(fd, 0x000f, 0x0000);
			UsbSendVendorRequest(fd, 0x000b, 0x0000);
			UsbSendVendorRequest(fd, 0x000f, 0x0000);
			UsbSendVendorRequest(fd, 0x000b, 0x0000);
			UsbSendVendorRequest(fd, 0x000f, 0x0000);
			UsbSendVendorRequest(fd, 0x000b, 0x0000);
			UsbSendVendorRequest(fd, 0x000f, 0x0000);
			UsbSendVendorRequest(fd, 0x000b, 0x0000);
			UsbSendVendorRequest(fd, 0x000f, 0x0000);
			UsbSendVendorRequest(fd, 0x0003, 0x0000);
			UsbSendVendorRequest(fd, 0x0007, 0x0000);
			UsbSendVendorRequest(fd, 0x0003, 0x0000);
			UsbSendVendorRequest(fd, 0x0007, 0x0000);
			UsbSendVendorRequest(fd, 0x0003, 0x0000);
			UsbSendVendorRequest(fd, 0x0007, 0x0000);
			UsbSendVendorRequest(fd, 0x0003, 0x0000);
			UsbSendVendorRequest(fd, 0x0007, 0x0000);
			UsbSendVendorRequest(fd, 0x0003, 0x0000);
			UsbSendVendorRequest(fd, 0x0007, 0x0000);
			UsbSendVendorRequest(fd, 0x0003, 0x0000);
			UsbSendVendorRequest(fd, 0x0007, 0x0000);
			UsbSendVendorRequest(fd, 0x0003, 0x0000);
			UsbSendVendorRequest(fd, 0x0007, 0x0000);
			UsbSendVendorRequest(fd, 0x0003, 0x0000);
			UsbSendVendorRequest(fd, 0x0007, 0x0000);
			UsbSendVendorRequest(fd, 0x000b, 0x0000);
			UsbSendVendorRequest(fd, 0x000f, 0x0000);
			UsbSendVendorRequest(fd, 0x000b, 0x0000);
			UsbSendVendorRequest(fd, 0x000f, 0x0000);
			UsbSendVendorRequest(fd, 0x000b, 0x0000);
			UsbSendVendorRequest(fd, 0x000f, 0x0000);
			UsbSendVendorRequest(fd, 0x000b, 0x0000);
			UsbSendVendorRequest(fd, 0x000f, 0x0000);
			UsbSendVendorRequest(fd, 0x000b, 0x0000);
			UsbSendVendorRequest(fd, 0x000f, 0x0000);
			UsbSendVendorRequest(fd, 0x000b, 0x0000);
			UsbSendVendorRequest(fd, 0x000f, 0x0000);
			UsbSendVendorRequest(fd, 0x000b, 0x0000);
			UsbSendVendorRequest(fd, 0x000f, 0x0000);
			UsbSendVendorRequest(fd, 0x000b, 0x0000);
			UsbSendVendorRequest(fd, 0x000f, 0x0000);
			UsbSendVendorRequest(fd, 0x0009, 0x0000);
			UsbSendVendorRequest(fd, 0x000d, 0x0000);
		}
		break;
	case BON_LED_GREEN:
		{
			UsbSendVendorRequest(fd, 0x000b, 0x0000);
			UsbSendVendorRequest(fd, 0x000f, 0x0000);
			UsbSendVendorRequest(fd, 0x0003, 0x0000);
			UsbSendVendorRequest(fd, 0x0007, 0x0000);
			UsbSendVendorRequest(fd, 0x0003, 0x0000);
			UsbSendVendorRequest(fd, 0x0007, 0x0000);
			UsbSendVendorRequest(fd, 0x000b, 0x0000);
			UsbSendVendorRequest(fd, 0x000f, 0x0000);
			UsbSendVendorRequest(fd, 0x000b, 0x0000);
			UsbSendVendorRequest(fd, 0x000f, 0x0000);
			UsbSendVendorRequest(fd, 0x0003, 0x0000);
			UsbSendVendorRequest(fd, 0x0007, 0x0000);
			UsbSendVendorRequest(fd, 0x0003, 0x0000);
			UsbSendVendorRequest(fd, 0x0007, 0x0000);
			UsbSendVendorRequest(fd, 0x000b, 0x0000);
			UsbSendVendorRequest(fd, 0x000f, 0x0000);
			UsbSendVendorRequest(fd, 0x0003, 0x0000);
			UsbSendVendorRequest(fd, 0x0007, 0x0000);
			UsbSendVendorRequest(fd, 0x0003, 0x0000);
			UsbSendVendorRequest(fd, 0x0007, 0x0000);
			UsbSendVendorRequest(fd, 0x0003, 0x0000);
			UsbSendVendorRequest(fd, 0x0007, 0x0000);
			UsbSendVendorRequest(fd, 0x0003, 0x0000);
			UsbSendVendorRequest(fd, 0x0007, 0x0000);
			UsbSendVendorRequest(fd, 0x0003, 0x0000);
			UsbSendVendorRequest(fd, 0x0007, 0x0000);
			UsbSendVendorRequest(fd, 0x0003, 0x0000);
			UsbSendVendorRequest(fd, 0x0007, 0x0000);
			UsbSendVendorRequest(fd, 0x0003, 0x0000);
			UsbSendVendorRequest(fd, 0x0007, 0x0000);
			UsbSendVendorRequest(fd, 0x0003, 0x0000);
			UsbSendVendorRequest(fd, 0x0007, 0x0000);
			UsbSendVendorRequest(fd, 0x0003, 0x0000);
			UsbSendVendorRequest(fd, 0x0007, 0x0000);
			UsbSendVendorRequest(fd, 0x0003, 0x0000);
			UsbSendVendorRequest(fd, 0x0007, 0x0000);
			UsbSendVendorRequest(fd, 0x000b, 0x0000);
			UsbSendVendorRequest(fd, 0x000f, 0x0000);
			UsbSendVendorRequest(fd, 0x000b, 0x0000);
			UsbSendVendorRequest(fd, 0x000f, 0x0000);
			UsbSendVendorRequest(fd, 0x000b, 0x0000);
			UsbSendVendorRequest(fd, 0x000f, 0x0000);
			UsbSendVendorRequest(fd, 0x000b, 0x0000);
			UsbSendVendorRequest(fd, 0x000f, 0x0000);
			UsbSendVendorRequest(fd, 0x000b, 0x0000);
			UsbSendVendorRequest(fd, 0x000f, 0x0000);
			UsbSendVendorRequest(fd, 0x000b, 0x0000);
			UsbSendVendorRequest(fd, 0x000f, 0x0000);
			UsbSendVendorRequest(fd, 0x000b, 0x0000);
			UsbSendVendorRequest(fd, 0x000f, 0x0000);
			UsbSendVendorRequest(fd, 0x000b, 0x0000);
			UsbSendVendorRequest(fd, 0x000f, 0x0000);
			UsbSendVendorRequest(fd, 0x0003, 0x0000);
			UsbSendVendorRequest(fd, 0x0007, 0x0000);
			UsbSendVendorRequest(fd, 0x000b, 0x0000);
			UsbSendVendorRequest(fd, 0x000f, 0x0000);
			UsbSendVendorRequest(fd, 0x000b, 0x0000);
			UsbSendVendorRequest(fd, 0x000f, 0x0000);
			UsbSendVendorRequest(fd, 0x0003, 0x0000);
			UsbSendVendorRequest(fd, 0x0007, 0x0000);
			UsbSendVendorRequest(fd, 0x0003, 0x0000);
			UsbSendVendorRequest(fd, 0x0007, 0x0000);
			UsbSendVendorRequest(fd, 0x000b, 0x0000);
			UsbSendVendorRequest(fd, 0x000f, 0x0000);
			UsbSendVendorRequest(fd, 0x0003, 0x0000);
			UsbSendVendorRequest(fd, 0x0007, 0x0000);
			UsbSendVendorRequest(fd, 0x0003, 0x0000);
			UsbSendVendorRequest(fd, 0x0007, 0x0000);
			UsbSendVendorRequest(fd, 0x0001, 0x0000);
			UsbSendVendorRequest(fd, 0x0005, 0x0000);
		}
		break;
	case BON_LED_RED:
		{
		}
		break;
	case BON_LED_WHITE:
		{
		}
		break;
	}
}

/**
 * 初期化処理 : 固定処理B
 * @param fd 対象ファイルディスクリプタ
 */
void
FriioWhite::UsbProcFixInitB(int fd)
{
	UsbSendVendorRequest(fd, 0x3040, 0x0001);
	UsbSendVendorRequest(fd, 0x3038, 0x0004);
	UsbSendVendorRequest(fd, 0x3040, 0x0005);
	UsbSendVendorRequest(fd, 0x3040, 0x0007);
	UsbSendVendorRequest(fd, 0x304f, 0x000f);
	UsbSendVendorRequest(fd, 0x3021, 0x0011);
	UsbSendVendorRequest(fd, 0x300b, 0x0012);
	UsbSendVendorRequest(fd, 0x302f, 0x0013);
	UsbSendVendorRequest(fd, 0x3031, 0x0014);
	UsbSendVendorRequest(fd, 0x3002, 0x0016);
	UsbSendVendorRequest(fd, 0x30c4, 0x0021);
	UsbSendVendorRequest(fd, 0x3020, 0x0022);
	UsbSendVendorRequest(fd, 0x3079, 0x002c);
	UsbSendVendorRequest(fd, 0x3034, 0x002d);
	UsbSendVendorRequest(fd, 0x3000, 0x002f);
	UsbSendVendorRequest(fd, 0x3028, 0x0030);
	UsbSendVendorRequest(fd, 0x3031, 0x0031);
	UsbSendVendorRequest(fd, 0x30df, 0x0032);
	UsbSendVendorRequest(fd, 0x3001, 0x0038);
	UsbSendVendorRequest(fd, 0x3078, 0x0039);
	UsbSendVendorRequest(fd, 0x3033, 0x003b);
	UsbSendVendorRequest(fd, 0x3033, 0x003c);
	UsbSendVendorRequest(fd, 0x3090, 0x0048);
	UsbSendVendorRequest(fd, 0x3068, 0x0051);
	UsbSendVendorRequest(fd, 0x3038, 0x005e);
	UsbSendVendorRequest(fd, 0x3000, 0x0071);
	UsbSendVendorRequest(fd, 0x3008, 0x0072);
	UsbSendVendorRequest(fd, 0x3000, 0x0077);
	UsbSendVendorRequest(fd, 0x3021, 0x00c0);
	UsbSendVendorRequest(fd, 0x3010, 0x00c1);
	UsbSendVendorRequest(fd, 0x301a, 0x00e4);
	UsbSendVendorRequest(fd, 0x301f, 0x00ea);
	UsbSendVendorRequest(fd, 0x3000, 0x0077);
	UsbSendVendorRequest(fd, 0x3000, 0x0071);
	UsbSendVendorRequest(fd, 0x3000, 0x0071);
	UsbSendVendorRequest(fd, 0x300c, 0x0076);
}
