// $Id$
// 白黒共通インターフェース

#ifndef _RECORDABLE_H_
#define _RECORDABLE_H_

#include <inttypes.h>
#include <stdint.h>
#include <string>
#include <vector>

/**
 * チューナの種別
 */
typedef enum {
	TUNER_FRIIO_WHITE,
	TUNER_FRIIO_BLACK,
#ifdef HDUS
	TUNER_HDUS,
	TUNER_HDP,
#endif
	TUNER_TYPES_LEN,
} TunerType;

/**
 * チューナの種別名称
 */
extern const std::string tunerTypeName[TUNER_TYPES_LEN];

/**
 * VHF, CATV, UHF, BS, CS等の種別
 */
typedef enum {
	BAND_VHF,  // とりあえず。
	BAND_CATV, // とりあえず。
	BAND_UHF,  // 白凡
	BAND_BS,   // 黒凡
	BAND_CS,   // 黒凡
	BAND_TYPES_LEN,
} BandType;

/**
 * チューナのインターフェース
 */
class Recordable
{
public:
	/**
	 * デストラクタ
	 */
	virtual ~Recordable() {
	};
	
	/**
	 * チューナを開く
	 * @param チューナが開けた場合true
	 */
	virtual const bool open(bool lnb) = 0;
	
	/**
	 * チューナを閉じる。
	 */
	virtual void close() = 0;
	
	/**
	 * ログ出力先を設定する。
	 * @param log ログ出力先
	 */
	virtual void setLog(std::ostream *log) = 0;
	
	/**
	 * チューナ検出用ロックファイル名を設定する。
	 * @param lockfile ファイル名
	 */
	virtual void setDetectLockFile(char *lockfile) = 0;
	
	/**
	 * チューナの種別を取得する。
	 * @return TunerType 種別
	 */
	virtual const TunerType getType() const = 0;
	
	/**
	 * 使用可能な周波数帯域を取得する。
	 * @return BandType 使用可能な周波数帯域
	 */
	virtual const std::vector<BandType> getSupportBands() const = 0;
	
	/**
	 * 周波数帯域/チャンネルを設定する。
	 * @param band 周波数帯域 UHF/BS/CS
	 * @param channel チャンネル
	 */
	virtual void setChannel(BandType band, int channel) = 0;
	
	/**
	 * 信号レベルを取得する。
	 * @return float 信号レベル
	 */
	virtual const float getSignalLevel(void) = 0;
	
	/**
	 * 受信開始
	 */
	virtual void startStream() = 0;
	
	/**
	 * 受信停止
	 */
	virtual void stopStream() = 0;
	
	/**
	 * データを取得する
	 * @param bufp データへのポインタを取得するポインタ(出力)
	 *             タイムアウト時NULL
	 * @param timeoutMsec データを待つ時間(msec)
	 *                    0の場合タイムアウト無し
	 * @return int 取得されたデータ長
	 */
	virtual int getStream(const uint8_t** bufp, int timeoutMsec) = 0;
	
	/**
	 * データを取得可能か？
	 * @return データが取得可能な場合true
	 */
	virtual bool isStreamReady() = 0;
	
	/**
	 * バッファをクリアする。
	 */
	virtual void clearBuffer() = 0;
};

/**
 * チューナーを生成する。
 * @param type チューナ種別
 * @return チューナー 失敗時NULL, deleteで開放。
 */
extern "C" Recordable* createRecordable(TunerType type);

#endif /* !defined(_RECORDABLE_H_) */
