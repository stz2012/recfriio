// $Id: FriioWhite.cpp 5689 2008-09-18 19:48:59Z clworld $
// 白凡

#include <glib.h>
#include <math.h>
#include "usbops.hpp"
#include "FriioWhiteWrapper.hpp"
#include "FriioExpress.hpp"
#include "FriioWhite.hpp"

FriioWhiteWrapper::FriioWhiteWrapper() : AbstractFriio()
{
  // 親でAbstractFriioが呼ばれた場合に備えて。
  supportBands.push_back(BAND_UHF);
  targetType         = TUNER_FRIIO_WHITE;
  asyncBufSize       = WHITE_ASYNCBUFFSIZE;
  requestReserveNum  = WHITE_REQUEST_RESERVE_NUM;
  requestPollingWait = WHITE_REQUEST_POLLING_WAIT;
  
  tuner = NULL;
}


static uint16_t usbinit_common[] = {
  0x40,0x01,0x0002,0x0011,0x0000,
  0x40,0x01,0x0000,0x0011,0x0000,
};

static uint16_t usbinit_expcheck_s1[] = {
  0x40,0x03,0x1200,0x0100,0x0002,0x03,0x80,
};

static uint16_t usbinit_expcheck_r1[] = {
  0xc0,0x02,0x1200,0x0100,0x0002,
};

static uint16_t usbinit_expcheck_s2[] = {
  0x40,0x03,0x9000,0x0100,0x0002,0x03,0x80,
};

static uint16_t usbinit_expcheck_r2[] = {
  0xc0,0x02,0x9000,0x0100,0x0002,
};

static uint16_t usbinit_expok[] = {
  0x40,0x01,0x0004,0x0030,0x0000,
};

static uint16_t usbinit_bw[] = {
  0x40,0x03,0x9000,0x0100,0x0002,0x03,0x80,
  0xc0,0x02,0x9000,0x0100,0x0002,
  0x40,0x03,0x3000,0x00fe,0x0001,0xc1,
};

static uint16_t usbinit_bwcheck[] = {
  0xc0,0x02,0x3000,0x0100,0x0001,
};

static uint16_t usbinit_black[] = {
  0xc0,0x02,0x3000,0x0080,0x0001,
};

static uint16_t usbinit_bwok[] = {
  0x40,0x01,0x0004,0x0030,0x0000,
};

#define DATALEN(a) (sizeof(a) / sizeof(uint16_t))

/**
 * 固定処理A(デバイス判定)
 * @param fd 対象ファイルディスクリプタ
 * @return TunerType チューナの種別
 */
TunerType
FriioWhiteWrapper::UsbProcFixInitA(int fd)
{
  TunerType type;
  uint8_t buf[2];
 
  /* 共通初期化処理 */
  usb_ctrl_sends(fd,  usbinit_common, DATALEN(usbinit_common), NULL, 0);

  /* Express判定 
     0x1200と0x9000に送信して、双方とも0xff,0xffの場合は白か黒。
     Expressの場合は、片方0x01,0xb3が帰って来る */
  usb_ctrl_sends(fd, usbinit_expcheck_s1, DATALEN(usbinit_expcheck_s1), NULL, 0);
  usb_ctrl_sends(fd, usbinit_expcheck_r1, DATALEN(usbinit_expcheck_r1), buf, 2);
  
  /* Expressは0x01,0xb3が帰って来る */
  if(buf[0] != 0xff && buf[1] != 0xff) {
    /* Epress終了処理 */
    usb_ctrl_sends(fd, usbinit_expok, DATALEN(usbinit_expok), NULL, 0);
    /* tuner初期化 */
    tuner = new FriioExpress();
    copyinstance(fd);
    /* 扱いとしてはWHITEと一緒 */
    type = TUNER_FRIIO_WHITE;
    return type;
  }

  usb_ctrl_sends(fd, usbinit_expcheck_s2, DATALEN(usbinit_expcheck_s2), NULL, 0);
  usb_ctrl_sends(fd, usbinit_expcheck_r2, DATALEN(usbinit_expcheck_r2), buf, 2);
  
  /* Expressは0x01,0xb3が帰って来る */
  if(buf[0] != 0xff && buf[1] != 0xff) {
    /* Epress終了処理 */
    usb_ctrl_sends(fd, usbinit_expok, DATALEN(usbinit_expok), NULL, 0);
    /* tuner初期化 */
    tuner = new FriioExpress();
    copyinstance(fd);
    /* 扱いとしてはWHITEと一緒 */
    type = TUNER_FRIIO_WHITE;
    return type;
  }

  /* 白黒共通初期化処理 */
  usb_ctrl_sends(fd, usbinit_bw, DATALEN(usbinit_bw), NULL, 0);
  
  /* 白黒判定 */
  usb_ctrl_sends(fd, usbinit_bwcheck, DATALEN(usbinit_bwcheck), buf, 1);

  /* 黒は0x00が帰って来る */
  if(buf[0] == 0x00) {
    type = TUNER_FRIIO_BLACK;
    /* 黒は以下が追加されるみたい */
    usb_ctrl_sends(fd, usbinit_black, DATALEN(usbinit_black), NULL, 0);
  } else {
    /* tuner初期化 */
    tuner = new FriioWhite();
    copyinstance(fd);
    type = TUNER_FRIIO_WHITE;
  }
  
  /* 白黒共通終了処理 = Epress終了処理の最後 */
  usb_ctrl_sends(fd, usbinit_bwok, DATALEN(usbinit_bwok), NULL, 0);
  
  return type;
}

void
FriioWhiteWrapper::copyinstance(int fd)
{
  /* 
     元々の設計は、機器(friio)毎に完全に分離されているクラス構成
     Expressのように白扱いしたい機器が複数あると、困る
     元のコードをいじるのは嫌だったので、仮想機器としてWrapperクラスを
     作った。
     以下のコードは、元々の設計を無理やり直した歪み
   */
  tuner->CopyWrapper(true, log, lockfile, fd, endpoint);
}
