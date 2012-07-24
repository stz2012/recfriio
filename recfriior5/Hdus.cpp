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
#include "Hdus.hpp"
#include "decrypt.c"

static uint8_t xorTable[] = {
  0x6D, 0x4E, 0xF0, 0x1E, 0x59, 0x8B, 0xB2, 0xF0, 0x31, 0xF9, 0xD0, 0x85, 0xDD, 0x84, 0x4A, 0x6C,
  0x15, 0x1C, 0x1C, 0x0A, 0x5B, 0x61, 0x6F, 0x1A, 0xAD, 0x60, 0x46, 0xF2, 0xD5, 0x03, 0x91, 0xC7,
  0xB7, 0xDE, 0x68, 0x9A, 0x07, 0x47, 0x62, 0x5E, 0x39, 0xD6, 0x7E, 0x45, 0x10, 0x9D, 0x1A, 0xC4,
  0x4C, 0x7F, 0xB7, 0x03, 0xCC, 0xAD, 0x72, 0x6C, 0xE9, 0x1B, 0x85, 0xA8, 0xEE, 0x4B, 0xA9, 0x3A,
  0xFF, 0xF1, 0x3B, 0x16, 0xE0, 0x59, 0x4C, 0x3A, 0xD5, 0x4A, 0x14, 0x95, 0x0A, 0xD4, 0x25, 0x23,
  0xF6, 0x2F, 0xE3, 0x57, 0x35, 0xCE, 0x52, 0x84, 0x11, 0xA3, 0xCB, 0x34, 0xA7, 0x15, 0x60, 0xE1,
  0x98, 0x71, 0xF4, 0x1F, 0xF7, 0xBD, 0x22, 0x4B, 0x61, 0x8A, 0xD6, 0xAF, 0x24, 0x27, 0xC3, 0xA7,
  0x07, 0x9B, 0xA5, 0x57, 0x83, 0x80, 0x58, 0x07, 0x1D, 0xDE, 0x0A, 0xD8, 0xB9, 0x0A, 0xBE, 0xAC,
  0xA9, 0x76, 0x00, 0x1C, 0x23, 0x9F, 0x82, 0x79, 0x41, 0x56, 0xC8, 0x83, 0xC3, 0x38, 0x37, 0x4B,
  0x5A, 0xED, 0xC9, 0xD5, 0xF2, 0x87, 0x02, 0xC3, 0x79, 0xA0, 0x61, 0xD9, 0x63, 0x50, 0xCA, 0x72,
  0xC2, 0x51, 0x40, 0x98, 0xD9, 0x8B, 0xED, 0xA9, 0xA1, 0x4C, 0xA5, 0xE7, 0xAF, 0xF2, 0x08, 0x4D,
  0x82, 0xF8, 0xD5, 0x02, 0x61, 0xD8, 0xC2, 0xE1
};

Hdus::Hdus()
    : AbstractFriio(), saddr( 0 ), buffer( NULL )
{
    supportBands.push_back(BAND_CATV);
    supportBands.push_back(BAND_UHF);
    targetType         = TUNER_HDUS;
    asyncBufSize       = HDUS_ASYNCBUFFSIZE;
    requestReserveNum  = HDUS_REQUEST_RESERVE_NUM;
    requestPollingWait = HDUS_REQUEST_POLLING_WAIT;
}


Hdus::~Hdus()
{
    if( initialized ) close();
    if( buffer ) free( buffer );
    buffer = NULL;
}


/**
 * 周波数帯域/チャンネルを設定する。
 * @param newBand 周波数帯域 UHF/BS/CS
 * @param newChannel チャンネル
 */
void
Hdus::setChannel(BandType newBand, int newChannel)
{
    if( ! saddr ) return;

    int ugen = tunerFd;
    uint8_t data[0x200];
    int chan = newChannel, freq, val;
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
    val = (freq + 57) * 7;
    usleep (50 * 1000);
    send_ctl(data, ugen, 0x0d, 0xfe00, (val & 0xff00) | 0xc0, 0x0004);
    send_ctl(data, ugen, 0x0d, ((val & 0xff) << 8) | 0x03,
             (freq < 420) ? 0x8291 : 0x8491, 0x0004);
    send_ctl(data, ugen, 0x0d, 0xe306, 0x0000, 0x0002);
    send_ctl(data, ugen, 0x0e, 0x0020, 0x0000, 0x0008);
    send_ctl(data, ugen, 0x03, 0x0120, 0x0040, 0x0002);

    send_ctl(data, ugen, 0x09, 0x0100, 0x0000, 0x0001);
    if (r5 == 0x91) {
        send_ctl(data, ugen, 0x04, 0x0000, 0x0000, 0x0042);
        val = data[65];
        send_ctl(data, ugen, 0x05, 0x003f, (val & 0xff) | 0x04, 0x0003);
        send_ctl(data, ugen, 0x05, 0x0f40, 0x0000, 0x0002);
    }

    /* Set PID filter */
    send_ctl(data, ugen, 0x04, 0x0040, 0x0000, 0x0002);
    if (r5 == 0x91) {
        send_ctl(data, ugen, 0x05, ((data[1] | 0x03) << 8) | 0x40, 0x0000, 0x0002);
    } else {
        send_ctl(data, ugen, 0x05, ((data[1] | 0x0f) << 8) | 0x40, 0x0000, 0x0002);
    }
    send_ctl(data, ugen, 0x05, (xpid_min & 0xff00) | 0x41,
             (xpid_min & 0xff), 0x0003);
    send_ctl(data, ugen, 0x05, (xpid_max & 0xff00) | 0x43,
             (xpid_max & 0xff), 0x0003);

    /* kick ASV5211 bulk transfer */
    send_ctl(data, ugen, 0x06, 0x0000, 0x0000, 0x0001);
    send_ctl(data, ugen, 0x04, 0x0040, 0x0000, 0x0002);
    send_ctl(data, ugen, 0x05, 0x0f40, 0x0000, 0x0002);
    usleep (150 * 1000);
}

/**
 * 信号レベルを取得する。
 * @return float 信号レベル
 */
const float
Hdus::getSignalLevel(void)
{
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
Hdus::getStream(const uint8_t** bufp, int timeoutMsec)
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
        if (r5 == 0x91) {
            // XOR
            for( int i = 4; i < 188; i++)
                buf[i] ^= xorTable[i - 4];
        } else {
            // DES
            static unsigned char tmp[188];
            memcpy(tmp, buf, 188);
            packetDecrypt(tmp, buf, 1);
        }

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
Hdus::is_friio(const std::string &devfile)
{
    usb_device_descriptor usb_desc;
    try {
        usb_getdesc(devfile.c_str(), &usb_desc);
    } catch (usb_error &e) {
        if (log) *log << "usb_getdesc: " << e.what() << std::endl;
        return false;
    }
    
    if ( TARGET_ID_VENDOR_HDUS == usb_desc.idVendor && TARGET_ID_PRODUCT_HDUS == usb_desc.idProduct) {
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
Hdus::UsbProcFixInitA(int fd)
{
    return TUNER_HDUS;
}


/**
 * 初期化処理 : 処理開始
 * 便宜上Beginにしているが、本来どんなことをしているかは不明
 * @param fd 対象ファイルディスクリプタ
 */
void
Hdus::UsbProcBegin(int fd)
{
    if( ! buffer ){
        buffer = (uint8_t*)malloc( HDUS_ASYNCBUFFSIZE * TSDATASIZE + 1024 );
    }

    buffersize = 0;
    buffer_remainsize = 0;

    /* HDUS を開く */

    if (log) *log << "opening HDUS...";

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
    usleep (50 * 1000);
    send_ctl(data, ugen, 0x08, 0x0100, 0x0000, 0x0001);
    usleep (100 * 1000);
    send_ctl(data, ugen, 0x08, 0xdd0c, 0x0000, 0x0001);
    usleep (100 * 1000);

    send_ctl(data, ugen, 0x17, 0x0002, 0x0000, 0x0003);

    /* Tuner on */
    send_ctl(data, ugen, 0x08, 0xd500, 0x0000, 0x0001);
    usleep (50 * 1000);
    send_ctl(data, ugen, 0x08, 0x0800, 0x0000, 0x0001);
    send_ctl(data, ugen, 0x08, 0x4040, 0x0000, 0x0001);
    usleep (5 * 1000);
    send_ctl(data, ugen, 0x08, 0x8080, 0x0000, 0x0001);
    usleep (50 * 1000);
    send_ctl(data, ugen, 0x08, 0x0404, 0x0000, 0x0001);
    usleep (50 * 1000);

    /* Tuner init */
    send_ctl(data, ugen, 0x02, 0x00a8, 0x0000, 0x0002);
    /* */
    send_ctl(data, ugen, 0x03, 0x0120, 0x0000, 0x0002);
    send_ctl(data, ugen, 0x03, 0x0220, 0x0000, 0x0002);
    send_ctl(data, ugen, 0x03, 0x0320, 0x0000, 0x0002);
    send_ctl(data, ugen, 0x03, 0x0420, 0x0000, 0x0002);
    send_ctl(data, ugen, 0x03, 0x0520, 0x0000, 0x0002);
    send_ctl(data, ugen, 0x03, 0x0620, 0x0000, 0x0002);
    send_ctl(data, ugen, 0x03, 0x0720, 0x0000, 0x0002);
    send_ctl(data, ugen, 0x03, 0x0820, 0x0000, 0x0002);
    send_ctl(data, ugen, 0x03, 0x0c20, 0x0000, 0x0002);
    send_ctl(data, ugen, 0x03, 0x1120, 0x0021, 0x0002);
    send_ctl(data, ugen, 0x03, 0x1220, 0x000b, 0x0002);
    send_ctl(data, ugen, 0x03, 0x1320, 0x002c, 0x0002);
    send_ctl(data, ugen, 0x03, 0x1420, 0x000a, 0x0002);
    send_ctl(data, ugen, 0x03, 0x1520, 0x0040, 0x0002);
    send_ctl(data, ugen, 0x03, 0x1620, 0x000a, 0x0002);
    send_ctl(data, ugen, 0x03, 0x1720, 0x0070, 0x0002);
    send_ctl(data, ugen, 0x03, 0x1820, 0x0031, 0x0002);
    send_ctl(data, ugen, 0x03, 0x1920, 0x0013, 0x0002);
    send_ctl(data, ugen, 0x03, 0x1a20, 0x0031, 0x0002);
    send_ctl(data, ugen, 0x03, 0x1b20, 0x0013, 0x0002);
    send_ctl(data, ugen, 0x03, 0x1c20, 0x002a, 0x0002);
    send_ctl(data, ugen, 0x03, 0x1d20, 0x00aa, 0x0002);
    send_ctl(data, ugen, 0x03, 0x1e20, 0x00a2, 0x0002);
    send_ctl(data, ugen, 0x03, 0x1f20, 0x0008, 0x0002);
    send_ctl(data, ugen, 0x03, 0x2020, 0x0000, 0x0002);
    send_ctl(data, ugen, 0x03, 0x2120, 0x00ff, 0x0002);
    send_ctl(data, ugen, 0x03, 0x2220, 0x0080, 0x0002);
    send_ctl(data, ugen, 0x03, 0x2320, 0x004c, 0x0002);
    send_ctl(data, ugen, 0x03, 0x2420, 0x004c, 0x0002);
    send_ctl(data, ugen, 0x03, 0x2520, 0x0000, 0x0002);
    send_ctl(data, ugen, 0x03, 0x2620, 0x0000, 0x0002);
    send_ctl(data, ugen, 0x03, 0x2720, 0x000c, 0x0002);
    send_ctl(data, ugen, 0x03, 0x2820, 0x0060, 0x0002);
    send_ctl(data, ugen, 0x03, 0x2920, 0x006b, 0x0002);
    send_ctl(data, ugen, 0x03, 0x2a20, 0x0040, 0x0002);
    send_ctl(data, ugen, 0x03, 0x2b20, 0x0040, 0x0002);
    send_ctl(data, ugen, 0x03, 0x2c20, 0x00ff, 0x0002);
    send_ctl(data, ugen, 0x03, 0x2d20, 0x0000, 0x0002);
    send_ctl(data, ugen, 0x03, 0x2e20, 0x00ff, 0x0002);
    send_ctl(data, ugen, 0x03, 0x2f20, 0x0000, 0x0002);
    send_ctl(data, ugen, 0x03, 0x3020, 0x0028, 0x0002);
    send_ctl(data, ugen, 0x03, 0x3120, 0x000f, 0x0002);
    send_ctl(data, ugen, 0x03, 0x3220, 0x00a0, 0x0002);
    send_ctl(data, ugen, 0x03, 0x3420, 0x003f, 0x0002);
    send_ctl(data, ugen, 0x03, 0x3820, 0x0001, 0x0002);
    send_ctl(data, ugen, 0x03, 0x3920, 0x0070, 0x0002);
    send_ctl(data, ugen, 0x03, 0x3a20, 0x002d, 0x0002);
    send_ctl(data, ugen, 0x03, 0x3b20, 0x0021, 0x0002);
    send_ctl(data, ugen, 0x03, 0x3c20, 0x003f, 0x0002);
    send_ctl(data, ugen, 0x03, 0x3d20, 0x0010, 0x0002);
    send_ctl(data, ugen, 0x03, 0x3e20, 0x0008, 0x0002);
    send_ctl(data, ugen, 0x03, 0x3f20, 0x000c, 0x0002);
    send_ctl(data, ugen, 0x03, 0x4020, 0x000c, 0x0002);
    send_ctl(data, ugen, 0x03, 0x4120, 0x0000, 0x0002);
    send_ctl(data, ugen, 0x03, 0x4220, 0x0000, 0x0002);
    send_ctl(data, ugen, 0x03, 0x4320, 0x004f, 0x0002);
    send_ctl(data, ugen, 0x03, 0x4420, 0x00ff, 0x0002);
    send_ctl(data, ugen, 0x03, 0x4620, 0x0020, 0x0002);
    send_ctl(data, ugen, 0x03, 0x4720, 0x0000, 0x0002);
    send_ctl(data, ugen, 0x03, 0x4820, 0x0090, 0x0002);
    send_ctl(data, ugen, 0x03, 0x4920, 0x00e6, 0x0002);
    send_ctl(data, ugen, 0x03, 0x4a20, 0x0002, 0x0002);
    send_ctl(data, ugen, 0x03, 0x4b20, 0x0054, 0x0002);
    send_ctl(data, ugen, 0x03, 0x4c20, 0x0000, 0x0002);
    send_ctl(data, ugen, 0x03, 0x5020, 0x0000, 0x0002);
    send_ctl(data, ugen, 0x03, 0x5120, 0x0058, 0x0002);
    send_ctl(data, ugen, 0x03, 0x5220, 0x0020, 0x0002);
    send_ctl(data, ugen, 0x03, 0x5420, 0x0057, 0x0002);
    send_ctl(data, ugen, 0x03, 0x5520, 0x00f1, 0x0002);
    send_ctl(data, ugen, 0x03, 0x5620, 0x0020, 0x0002);
    send_ctl(data, ugen, 0x03, 0x5720, 0x0070, 0x0002);
    send_ctl(data, ugen, 0x03, 0x5820, 0x0060, 0x0002);
    send_ctl(data, ugen, 0x03, 0x5c20, 0x0050, 0x0002);
    send_ctl(data, ugen, 0x03, 0x5d20, 0x0000, 0x0002);
    send_ctl(data, ugen, 0x03, 0x5f20, 0x0080, 0x0002);
    send_ctl(data, ugen, 0x03, 0x7020, 0x0018, 0x0002);
    send_ctl(data, ugen, 0x03, 0x7120, 0x0000, 0x0002);
    send_ctl(data, ugen, 0x03, 0x7220, 0x0000, 0x0002);
    send_ctl(data, ugen, 0x03, 0x7520, 0x0022, 0x0002);
    send_ctl(data, ugen, 0x03, 0x7620, 0x0000, 0x0002);
    send_ctl(data, ugen, 0x03, 0x7720, 0x0001, 0x0002);
    send_ctl(data, ugen, 0x03, 0x7c20, 0x0000, 0x0002);
    send_ctl(data, ugen, 0x03, 0x7d20, 0x0052, 0x0002);
    send_ctl(data, ugen, 0x03, 0xba20, 0x0000, 0x0002);
    send_ctl(data, ugen, 0x03, 0xbb20, 0x0000, 0x0002);
    send_ctl(data, ugen, 0x03, 0xbc20, 0x0000, 0x0002);
    send_ctl(data, ugen, 0x03, 0xc220, 0x0000, 0x0002);
    send_ctl(data, ugen, 0x03, 0xc720, 0x0000, 0x0002);
    send_ctl(data, ugen, 0x03, 0xe420, 0x001a, 0x0002);
    send_ctl(data, ugen, 0x03, 0xe920, 0x0008, 0x0002);
    send_ctl(data, ugen, 0x03, 0xec20, 0x0000, 0x0002);
    send_ctl(data, ugen, 0x03, 0xef20, 0x0000, 0x0002);
    /* */
    send_ctl(data, ugen, 0x02, 0x00a8, 0x0000, 0x0002);

    /* ASIE5606 on */
    send_ctl(data, ugen, 0x08, 0x11ff, 0x0000, 0x0001);
    usleep (50 * 1000);
    send_ctl(data, ugen, 0x08, 0x1100, 0x0000, 0x0001);
    usleep (50 * 1000);
    send_ctl(data, ugen, 0x08, 0x11ff, 0x0000, 0x0001);
    usleep (100 * 1000);

    send_ctl(data, ugen, 0x02, 0x0900 | saddr, 0x0000, 0x0002);
    if (data[0] != 1) {
        saddr = 0x4a; /* HDUSF */
        send_ctl(data, ugen, 0x02, 0x0900 | saddr, 0x0000, 0x0002);
        if (data[0] != 1) {
            // error
        }
    }
    if ( (data[1] & 7) == 6) {
        r5 = 0x80; /* DES */
    }

    /* ASV5211 is high speed */
    send_ctl(data, ugen, 0x0a, 0x0000, 0x0000, 0x0001);
    usleep (30 * 1000);
    /* ASIE decrypt mode configuration */
    send_ctl(data, ugen, 0x03, 0x0500 | saddr, r5, 0x0002);

    if (r5 == 0x91) {
        /* Set XOR values which is used against XOR table */
        for (val = 0x1000; val < 0x2000; val += 0x0100)
            send_ctl(data, ugen, 0x03, (val & 0xff00) | saddr, 0x00, 0x0002);
    } else {
        /* Set key */
        for (val = 0x1000; val < 0x2000; val += 0x0100)
            send_ctl(data, ugen, 0x03, val | saddr, 0x0000, 0x0002);
    }

    if (log) *log << "done." << std::endl;
}

/**
 * 処理終了
 * 便宜上Endにしている。恐らくバッファクリア
 * @param fd 対象ファイルディスクリプタ
 */
void
Hdus::UsbProcEnd(int fd)
{
    if( ! saddr ) return;

    if (log) *log << "closing HDUS...";

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
    usleep (50 * 1000);
    send_ctl(data, ugen, 0x08, 0x0100, 0x0000, 0x0001);
    usleep (100 * 1000);
    send_ctl(data, ugen, 0x08, 0xdd0c, 0x0000, 0x0001);
    usleep (100 * 1000);

    saddr = 0;

    if (log) *log << "done." << std::endl;
}

/**
 * 初期化処理 : ストリーム制御データ
 * @param fd 対象ファイルディスクリプタ
 */
void
Hdus::UsbProcStreamInit(int fd)
{}

/**
 * ＬＥＤ制御
 * @param fd 対象ファイルディスクリプタ
 * @param color 色
 */
void
Hdus::UsbProcLED(int fd, BonLedColor color, bool lnb_powered)
{}

/**
 * 初期化処理 : 固定処理B
 * @param fd 対象ファイルディスクリプタ
 */
void
Hdus::UsbProcFixInitB(int fd)
{}



void
Hdus::send_ctl(uint8_t *data, int fd, int req, int val, int idx, int len)
{
//    fprintf( stderr, "fd = 0x%x, req = 0x%x, val = 0x%x\n", fd, req, val );

    usbdevfs_ctrltransfer ctrl = {
        USB_DIR_IN | USB_TYPE_VENDOR | USB_RECIP_DEVICE, req, val, idx, len, REQUEST_TIMEOUT, data
    };

    usb_ctrl(fd, &ctrl);
}
