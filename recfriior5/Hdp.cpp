/*-
 * Copyright (c) 2008 FUKAUMI Naoki.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <glib.h>
#include <math.h>
#include <stdio.h>
#include "usbops.hpp"
#include "Hdp.hpp"

#include "decrypt.c"

Hdp::Hdp()
    : AbstractFriio(), saddr( 0 ), buffer( NULL )
{
	supportBands.push_back(BAND_CATV);
	supportBands.push_back(BAND_UHF);
	targetType         = TUNER_HDP;
	asyncBufSize       = HDP_ASYNCBUFFSIZE;
	requestReserveNum  = HDP_REQUEST_RESERVE_NUM;
	requestPollingWait = HDP_REQUEST_POLLING_WAIT;
}


Hdp::~Hdp()
{
    if( initialized ) close();
    if( buffer ) free( buffer );
    buffer = NULL;
}


static uint8_t tunerAddr [0x5e] = {
  0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
  0x0C, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
  0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
  0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
  0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
  0x30, 0x31, 0x32, 0x34, 0x38, 0x39, 0x3A, 0x3B,
  0x3C, 0x3D, 0x3E, 0x3F, 0x40, 0x41, 0x42, 0x43,
  0x44, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C,
  0x50, 0x51, 0x52, 0x54, 0x55, 0x56, 0x57, 0x58,
  0x5C, 0x5D, 0x5E, 0x5F, 0x70, 0x71, 0x72, 0x75,
  0x76, 0x77, 0x7C, 0x7D, 0xBA, 0xBB, 0xBC, 0xC2,
  0xC7, 0xE4, 0xE6, 0xE9, 0xEC, 0xEF
};

static uint8_t tunerData [0x5e] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x21, 0x10, 0x03, 0xE0, 0x42, 0x09, 0x20,
  0x29, 0x13, 0x29, 0x13, 0x2A, 0xAA, 0xAA, 0xA8,
  0x00, 0xFF, 0x80, 0x4C, 0x4C, 0x80, 0x00, 0x0C,
  0x60, 0x6B, 0x40, 0x40, 0xFF, 0x00, 0xFF, 0x00,
  0x20, 0x0F, 0x84, 0x0F, 0x00, 0x0F, 0x20, 0x21,
  0x3F, 0x10, 0x08, 0x0C, 0x0C, 0x00, 0x00, 0x4F,
  0xFF, 0x20, 0x00, 0x90, 0xE6, 0x02, 0x54, 0x00,
  0x04, 0x58, 0x20, 0x57, 0xF1, 0x20, 0x70, 0x60,
  0x50, 0x00, 0x01, 0x80, 0x18, 0x00, 0x00, 0x22,
  0x00, 0x01, 0x00, 0x52, 0x00, 0x00, 0x00, 0x10,
  0x00, 0x1A, 0x2F, 0x08, 0x00, 0x00
};

/*
 * send Tuner
 */
void 
Hdp::sendTuner(uint8_t *newdata, int len)
{
    newdata[0] = 0xfe; newdata[1] = 0xc0;
    uint8_t offset = 0;
    int ugen = tunerFd;
    uint8_t data[0x200];

    for (; len >= 3; len -= 3) {
        send_ctl(data, ugen, 0x0d, (newdata[0] << 8) | offset, (newdata[2] << 8) | newdata[1], 0x0004);
        newdata += 3;
        offset += 3;
    }
    switch ( len ) {
        case 2:
            send_ctl(data, ugen, 0x0d, (newdata[0] << 8) | offset, newdata[1], 0x0003);
            offset += 2;
            break;
        case 1:
            send_ctl(data, ugen, 0x0d, (newdata[0] << 8) | offset, 0x0000, 0x0002);
            offset += 1;
            break;
    }
    send_ctl(data, ugen, 0x0e, 0x0020, 0x0000, offset + 1);
}

/**
 * 周波数帯域/チャンネルを設定する。
 * @param newBand 周波数帯域 UHF/BS/CS
 * @param newChannel チャンネル
 */
void
Hdp::setChannel(BandType newBand, int newChannel)
{
    if( ! saddr ) return;

    int ugen = tunerFd;
    uint8_t data[0x200];
    int chan = newChannel, freq;
    int xpid_min = 0x1fff, xpid_max = 0x1fff;

    if (BAND_UHF != newBand || newChannel < 13 || newChannel > 62) {
        if(BAND_CATV != newBand || newChannel < 13 || newChannel > 63) {
            throw traceable_error("unknown channel."); // TODO: 適当なエラークラス
        } else {
            // CATV band
	    if (newChannel <= 22) {
                freq = 111 + (newChannel - 13) * 6;
	    } else if (newChannel == 23) {
		freq = 225;
	    } else if (newChannel <= 27) {
		freq = 233 + (newChannel - 24) * 6;
            } else {
		freq = 225 + (newChannel - 23) * 6;
	    }
        }
    } else {
        // UHF band
        freq = 473 + (chan - 13) * 6;
    }
    if (log) *log << "freq = " << freq << std::endl;


    /* Set frequency */
    freq = ((freq * 1000) << 6) / 1000;
    send_ctl(data, ugen, 0x02, 0x9d20, 0x0000, 0x0002);
    send_ctl(data, ugen, 0x03, 0x1c20, (data[1] & 0xfb) | 0x38, 0x0002);

    send_ctl(data, ugen, 0x03, 0x2520, 0x0000, 0x0002);
    send_ctl(data, ugen, 0x03, 0x2320, 0x004d, 0x0002);

    send_ctl(data, ugen, 0x03, 0x1e20, 0x00aa, 0x0002);
    send_ctl(data, ugen, 0x03, 0x1f20, 0x00a8, 0x0002);
    {
        uint8_t newdata[4];
        newdata[2] = 0x11; newdata[3] = 0x00; sendTuner (newdata, 4);
        newdata[2] = 0x13; newdata[3] = 0x15; sendTuner (newdata, 4);

        newdata[2] = 0x14; newdata[3] = (uint8_t) freq; sendTuner (newdata, 4);
        newdata[2] = 0x15; newdata[3] = (uint8_t) (freq >> 8); sendTuner (newdata, 4);

        newdata[2] = 0x11; newdata[3] = 0x02; sendTuner (newdata, 4);
    }
    send_ctl(data, ugen, 0x03, 0x1e20, 0x00a2, 0x0002);
    send_ctl(data, ugen, 0x03, 0x1f20, 0x0008, 0x0002);

    send_ctl(data, ugen, 0x03, 0x0120, 0x0040, 0x0002);
    send_ctl(data, ugen, 0x03, 0x2320, 0x004c, 0x0002);

    send_ctl(data, ugen, 0x09, 0x0100, 0x0000, 0x0001);

    /* Set PID filter */
    send_ctl(data, ugen, 0x04, 0x0040, 0x0000, 0x0002);
    send_ctl(data, ugen, 0x05, ((data[1] | 0x0f) << 8) | 0x40, 0x0000, 0x0002);
    send_ctl(data, ugen, 0x05, (xpid_min & 0xff00) | 0x41,
             (xpid_min & 0xff), 0x0003);
    send_ctl(data, ugen, 0x05, (xpid_max & 0xff00) | 0x43,
             (xpid_max & 0xff), 0x0003);

    /* kick ASV5211 bulk transfer */
    send_ctl(data, ugen, 0x06, 0x0000, 0x0000, 0x0001);
    usleep (150 * 1000);
}

/**
 * 信号レベルを取得する。
 * @return float 信号レベル
 */
const float
Hdp::getSignalLevel(void)
{
#if 1
    /* HDUSと同じか不明なので、とりあえず THRESHOLD より大きい値を返しておく */
    return  SIGNALLEVEL_RETRY_THRESHOLD + 8;
#else
    uint32_t val;
    float p;
    int ugen = tunerFd;
    uint8_t data[0x200];

    send_ctl(data, ugen, 0x02, 0x8b20, 0x0000, 0x0002);
    val = data[1] << 16;
    send_ctl(data, ugen, 0x02, 0x8c20, 0x0000, 0x0002);
    val |= data[1] << 8;
    send_ctl(data, ugen, 0x02, 0x8d20, 0x0000, 0x0002);
    val |= data[1];

    p = 20.0 * log10(5505024.0 / (double)val);
    if (isinf(p))
        p = 0.0f;
    if (log) {
	double p2 = 10.0 * log10(5505024.0 / (double)val);
	double cnr = 0.000024 * p2 * p2 * p2 * p2 - 0.0016 * p2 * p2 * p2 + 0.0398 * p2 * p2 + 0.5491 * p2 + 3.0965;
	*log << "CNR: " << cnr << std::endl;
    }

    return p;
#endif
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
Hdp::getStream(const uint8_t** bufp, int timeoutMsec)
{
    if( ! buffer  ){
        *bufp = NULL;
        return 0;
    }

    const size_t tssize = 188;

    const int readsize = AbstractFriio::getStream( bufp, timeoutMsec );
    if( *bufp == NULL ) return readsize;
    if( ! readsize ) return readsize;

    /* XOR 処理 */

//    fprintf( stderr, "buffersize = %d, remainsize = %d", buffersize, buffer_remainsize );

    // 前回の処理の残りをコピー
    if( buffer_remainsize ){
        memmove( buffer, buffer + ( buffersize - buffer_remainsize ), buffer_remainsize );
        buffersize = buffer_remainsize;
        buffer_remainsize = 0;
    }
    else buffersize = 0;

    memcpy( buffer + buffersize, *bufp, readsize );
    *bufp = buffer;
    buffersize += readsize;

//    fprintf( stderr, " -> buffersize = %d, remainsize = %d", buffersize, buffer_remainsize );

    size_t pos = 0;
    while( 1 ){

        // 同期
        size_t pos_sync = pos;
        while( pos < buffersize && buffer[ pos ] != 0x47 ) ++pos;
        if( pos_sync != pos ){
            if (log) *log << "sync " << pos - pos_sync << " bytes" << std::endl;
        }
        if( buffersize - pos < tssize ){
            buffer_remainsize = buffersize - pos;
            break;
        }

	// decrpyt
        uint8_t *buf = buffer + pos;
	uint8_t tmp[188];
	memcpy(tmp, buf, 188);
	packetDecrypt(tmp, buf, 1);

        pos += tssize;
    }

//    fprintf( stderr, "-> pos = %d\n", pos );

    return pos;
}


/**
 * デバイスファイルがFriioの物かを確認する。
 * @param devfile デバイスファイル
 * @return Friioである場合true;
 */
bool
Hdp::is_friio(const std::string &devfile)
{
	usb_device_descriptor usb_desc;
	try {
		usb_getdesc(devfile.c_str(), &usb_desc);
	} catch (usb_error &e) {
		if (log) *log << "usb_getdesc: " << e.what() << std::endl;
		return false;
	}
	
	if ( TARGET_ID_VENDOR_HDU == usb_desc.idVendor && TARGET_ID_PRODUCT_HDU == usb_desc.idProduct) {
		if (log) *log << "HDP Type: HDU" << std::endl;
        	tunerCount = 1;
		return true;
	} else if ( TARGET_ID_VENDOR_HDUS == usb_desc.idVendor && TARGET_ID_PRODUCT_FS100U == usb_desc.idProduct) {
		if (log) *log << "HDP Type: LDT-FS100U" << std::endl;
        	tunerCount = 1;
		return true;
	} else if ( TARGET_ID_VENDOR_HDUS == usb_desc.idVendor && TARGET_ID_PRODUCT_HDP == usb_desc.idProduct) {
		if (log) *log << "HDP Type: HDP" << std::endl;
        	tunerCount = 1;
		return true;
	} else if ( TARGET_ID_VENDOR_HDUS == usb_desc.idVendor && TARGET_ID_PRODUCT_HDP2 == usb_desc.idProduct) {
		if (log) *log << "HDP Type: HDP2" << std::endl;
        	tunerCount = 2;
		return true;
	} else {
		return false;
	}
}


/**
 * 固定処理A(デバイス判定)
 * @param fd 対象ファイルディスクリプタ
 * @return TunerType チューナの種別
 */
TunerType
Hdp::UsbProcFixInitA(int fd)
{
    return TUNER_HDP;
}

/**
 * 初期化処理 : 処理開始
 * 便宜上Beginにしているが、本来どんなことをしているかは不明
 * @param fd 対象ファイルディスクリプタ
 */
void
Hdp::UsbProcBegin(int fd)
{
    if( ! buffer ){
        buffer = (uint8_t*)malloc( HDP_ASYNCBUFFSIZE * TSDATASIZE + 1024 );
    }

    buffersize = 0;
    buffer_remainsize = 0;

    /* HDP を開く */

    if (log) *log << "opening HDP...";

    int ugen = fd;
    uint8_t data[0x200];
    uint32_t val;
    r5 = 0x91;
    saddr = 0x5a;

    send_ctl(data, ugen, 0x0c, 0x0000, 0x0000, 0x003a);
    send_ctl(data, ugen, 0x17, 0x0002, 0x0000, 0x0003);
    send_ctl(data, ugen, 0x08, 0xfb07, 0x0000, 0x0001);
    send_ctl(data, ugen, 0x10, 0x0200, 0x0000, 0x0001);

    /* Power off */
    send_ctl(data, ugen, 0x08, 0xd400, 0x0000, 0x0001);
    send_ctl(data, ugen, 0x08, 0x0808, 0x0000, 0x0001);
    usleep (50 * 1000);

    send_ctl(data, ugen, 0x17, 0x0002, 0x0000, 0x0003);

    /* Tuner on */
    send_ctl(data, ugen, 0x08, 0x0808, 0x0000, 0x0001);
    usleep (50 * 1000);
    send_ctl(data, ugen, 0x08, 0x0800, 0x0000, 0x0001);
    usleep (150 * 1000);
    send_ctl(data, ugen, 0x08, 0x11ff, 0x0000, 0x0001);
    usleep (50 * 1000);
    send_ctl(data, ugen, 0x08, 0xd500, 0x0000, 0x0001);
    usleep (50 * 1000);
    send_ctl(data, ugen, 0x08, 0xd5ff, 0x0000, 0x0001);
    usleep (100 * 1000);

    /* Tuner init */
    uint8_t newdata[4];
    unsigned int i;
    newdata[2] = 0xff; sendTuner(newdata, 3);

    for (i = 0; i < sizeof(tunerAddr); i++) {
        send_ctl(data, ugen, 0x03, (tunerAddr[i] << 8) | 0x20, tunerData[i], 0x0002);
    }

    send_ctl(data, ugen, 0x02, 0x9d20, 0x0000, 0x0002);
    send_ctl(data, ugen, 0x03, 0x1c20, (data[1] & 0xeb) | 0x28, 0x0002);

    newdata[2] = 0x2c; newdata[3] = 0x44; sendTuner(newdata, 4);
    newdata[2] = 0x4d; newdata[3] = 0x40; sendTuner(newdata, 4);
    newdata[2] = 0x7f; newdata[3] = 0x02; sendTuner(newdata, 4);
    newdata[2] = 0x9a; newdata[3] = 0x52; sendTuner(newdata, 4);
    newdata[2] = 0x48; newdata[3] = 0x5a; sendTuner(newdata, 4);
    newdata[2] = 0x76; newdata[3] = 0x1a; sendTuner(newdata, 4);
    newdata[2] = 0x6a; newdata[3] = 0x48; sendTuner(newdata, 4);
    newdata[2] = 0x64; newdata[3] = 0x28; sendTuner(newdata, 4);
    newdata[2] = 0x66; newdata[3] = 0xe6; sendTuner(newdata, 4);
    newdata[2] = 0x35; newdata[3] = 0x11; sendTuner(newdata, 4);
    newdata[2] = 0x7e; newdata[3] = 0x01; sendTuner(newdata, 4);
    newdata[2] = 0x0b; newdata[3] = 0x99; sendTuner(newdata, 4);
    newdata[2] = 0x0c; newdata[3] = 0x00; sendTuner(newdata, 4);
    newdata[2] = 0x10; newdata[3] = 0x40; sendTuner(newdata, 4);

    newdata[2] = 0x12; newdata[3] = 0xca; sendTuner(newdata, 4);
    newdata[2] = 0x16; newdata[3] = 0x90; sendTuner(newdata, 4);
    newdata[2] = 0x32; newdata[3] = 0x36; sendTuner(newdata, 4);
    newdata[2] = 0xD8; newdata[3] = 0x18; sendTuner(newdata, 4);
    newdata[2] = 0x05; newdata[3] = 0x01; sendTuner(newdata, 4);

    /* ASIE5606 on */
    send_ctl(data, ugen, 0x08, 0x11ff, 0x0000, 0x0001);
    usleep (50 * 1000);
    send_ctl(data, ugen, 0x08, 0x1100, 0x0000, 0x0001);
    usleep (50 * 1000);
    send_ctl(data, ugen, 0x08, 0x11ff, 0x0000, 0x0001);
    usleep (100 * 1000);

    send_ctl(data, ugen, 0x02, 0x0900 | saddr, 0x0000, 0x0002);
    if (data[0] != 1) {
        saddr = 0x4a; /* HDP */
        send_ctl(data, ugen, 0x02, 0x0900 | saddr, 0x0000, 0x0002);
    }
    if ( (data[1] & 7) == 6) {
        r5 = 0x80; /* DES */
    }

    /* ASV5211 is high speed */
    send_ctl(data, ugen, 0x0a, 0x0000, 0x0000, 0x0001);
    usleep (30 * 1000);
    /* ASIE decrypt mode configuration */
    send_ctl(data, ugen, 0x03, 0x0500 | saddr, r5, 0x0002);

    /* Set key */
    for (val = 0x1000; val < 0x2000; val += 0x0100)
        send_ctl(data, ugen, 0x03, val | saddr, 0x0000, 0x0002);

    if (log) *log << "done." << std::endl;
}

/**
 * 処理終了
 * 便宜上Endにしている。恐らくバッファクリア
 * @param fd 対象ファイルディスクリプタ
 */
void
Hdp::UsbProcEnd(int fd)
{
    if( ! saddr ) return;

    if (log) *log << "closing HDP...";

    uint8_t data[0x200];
    int ugen = fd;

    /* Stop */
    usleep(100 * 1000);
    send_ctl(data, ugen, 0x07, 0x0000, 0x0000, 0x0001);

    send_ctl(data, ugen, 0x09, 0x0100, 0x0000, 0x0001);
    send_ctl(data, ugen, 0x03, 0x8000 | saddr, 0x0000, 0x0002);
    send_ctl(data, ugen, 0x03, 0x0400 | saddr, 0x0000, 0x0002);
    send_ctl(data, ugen, 0x03, 0x0000 | saddr, 0x0000, 0x0002);

    /* Power off */
    usleep (50 * 1000);
    send_ctl(data, ugen, 0x08, 0xd400, 0x0000, 0x0001);
    send_ctl(data, ugen, 0x08, 0x0808, 0x0000, 0x0001);
    usleep (50 * 1000);

    saddr = 0;

    if (log) *log << "done." << std::endl;
}

/**
 * 初期化処理 : ストリーム制御データ
 * @param fd 対象ファイルディスクリプタ
 */
void
Hdp::UsbProcStreamInit(int fd)
{}

/**
 * ＬＥＤ制御
 * @param fd 対象ファイルディスクリプタ
 * @param color 色
 */
void
Hdp::UsbProcLED(int fd, BonLedColor color, bool lnb_powered)
{}

/**
 * 初期化処理 : 固定処理B
 * @param fd 対象ファイルディスクリプタ
 */
void
Hdp::UsbProcFixInitB(int fd)
{}



void
Hdp::send_ctl(uint8_t *data, int fd, int req, int val, int idx, int len)
{
//    fprintf( stderr, "fd = 0x%x, req = 0x%x, val = 0x%x\n", fd, req, val );

    usbdevfs_ctrltransfer ctrl = {
        USB_DIR_IN | USB_TYPE_VENDOR | USB_RECIP_DEVICE, req, val, idx, len, REQUEST_TIMEOUT, data
    };
	
    usb_ctrl(fd, &ctrl);
}

bool
Hdp::vendorRequest(uint16_t wLength, uint8_t* data, uint8_t bRequest, uint8_t v0, uint8_t v1, uint8_t v2, uint8_t v3)
{
    send_ctl(data, tunerFd, bRequest, (v1 << 8) + v0, (v3 << 8) + v2, wLength);
    return true;
}
