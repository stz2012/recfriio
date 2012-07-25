// $Id$
// USB操作

#ifndef _USB_OPS_HPP_
#define _USB_OPS_HPP_

#include <linux/version.h>
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2, 4, 20)
#include <usb.h>
#include <linux/usb.h>
#elif LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 21)
#include <linux/usb_ch9.h>
#else
#include <linux/usb/ch9.h>
#endif
#include <linux/usbdevice_fs.h>
#include <stdint.h>
#include "error.hpp"

/**
 * usb_device_descriptorを取得する。
 * @param devfile デバイスファイル
 * @param desc usb_device_descriptorへのポインタ(出力)
 * @exception usb_error 失敗時
 */
void usb_getdesc(const char *devfile, usb_device_descriptor* desc) throw (usb_error);

/**
 * デバイスを開く。
 * @param devfile デバイスファイル
 * @return ファイルディスクリプタ
 * @exception usb_error 失敗時
 */
int usb_open(const char *devfile) throw (usb_error);

/**
 * 使用中か確認する。
 * @param fd 対象ファイルディスクリプタ
 * @param interface 対象インターフェース
 * @return 使用しているドライバ名 未使用時""
 * @exception usb_error USBのエラー時
 */
std::string usb_getdriver(int fd, int interface) throw (usb_error);

/**
 * setinterfaceする。
 * @param fd 対象ファイルディスクリプタ
 * @param interface 対象インターフェース
 * @exception usb_error USBのエラー時
 */
void usb_setinterface(int fd, int interface, int altsetting) throw (usb_error);


/**
 * claimする。
 * @param fd 対象ファイルディスクリプタ
 * @param interface 対象インターフェース
 * @exception usb_error USBのエラー時
 * @exception busy_error 使用中
 */
void usb_claim(int fd, unsigned int interface) throw (busy_error, usb_error);

/**
 * releaseする。
 * @param fd 対象ファイルディスクリプタ
 * @param interface 対象インターフェース
 * @exception usb_error USBのエラー時
 */
void usb_release(int fd, unsigned int interface) throw (usb_error);

/**
 * コントロールリクエストを送信する。
 * @param fd 対象ファイルディスクリプタ
 * @param ctrl コントロールリクエスト
 * @exception usb_error USBのエラー時
 */
int usb_ctrl(int fd, usbdevfs_ctrltransfer *ctrl) throw (usb_error);

/**
 * URBを送信する。
 * @param fd 対象ファイルディスクリプタ
 * @param urbp 送信するURBへのポインタ
 * @return int 結果(ioctl)
 */
int usb_submiturb(int fd, usbdevfs_urb* urbp);

/**
 * 完了したURBを受信する。
 * @param fd 対象ファイルディスクリプタ
 * @param urbpp 受信したURBを入れるポインタのポインタ
 * @return int 結果(ioctl)
 */
int usb_reapurb_ndelay(int fd, usbdevfs_urb** urbpp);

/**
 * 送信したURBをキャンセルする。
 * エラー処理未実施。
 * @param fd 対象ファイルディスクリプタ
 * @param urbp キャンセルするURBへのポインタ
 * @return int 結果(ioctl)
 */
int usb_discardurb(int fd, usbdevfs_urb* urbp);

/**
 * 配列で送信する。
 * @param fd 対象ファイルディスクリプタ
 * @param data    送信データの配列
 * @param length  送信データの配列数
 * @param rcvbuf  受信する場合のバッファ
 * @param rcv_len 受信バッファのサイズ
 * @exception usb_error USBのエラー時
 */
int usb_ctrl_sends(int fd, uint16_t data[], size_t length,  uint8_t *rcvbuf, size_t recv_len) throw (usb_error);

#endif /* !defined(_USB_OPS_HPP_) */
