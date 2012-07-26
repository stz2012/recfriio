// $Id: B25Decoder.cpp 5663 2008-09-15 17:53:59Z clworld $
// arib25

#include <sstream>

#include "../arib25/arib_std_b25.h"
#include "../arib25/b_cas_card.h"

#define _REAL_B25_
#include "B25Decoder.hpp"

/**
 * B25Decoder
 * リソースを開放する。
 */
void
B25Decoder::close() throw ()
{
	if (b25 != NULL) {
		b25->release(b25);
		b25 = NULL;
	}

	if (bcas != NULL) {
		bcas->release(bcas);
		bcas = NULL;
	}
}

/**
 * デコーダの初期化を行なう。
 * @exception b25_error 初期化エラー
 */
void
B25Decoder::open() throw (b25_error)
{
	int stat = 0;
	
	// 初期化済の場合リソースを開放する。
	close();
	
	// b25作成
	b25 = create_arib_std_b25();
	if (b25 == NULL) throw b25_error("failed to create arib_std_b25.");
	
	// b25->set_multi2_round
	stat = b25->set_multi2_round(b25, round);
	if (stat < 0) {
		std::ostringstream stream;
		stream << "b25->set_multi2_round(" << round << ") failed. code=" << stat;
		throw b25_error(stream.str());
	}
	
	// b25->set_strip
	stat = b25->set_strip(b25, strip ? 1 : 0);
	if (stat < 0) {
		std::ostringstream stream;
		stream << "b25->set_strip(" << strip << ") failed. code=" << stat;
		throw b25_error(stream.str());
	}
	
	// b25->set_emm_proc
	stat = b25->set_emm_proc(b25, emm ? 1 : 0);
	if (stat < 0) {
		std::ostringstream stream;
		stream << "b25->set_emm_proc(" << emm << ") failed. code=" << stat;
		throw b25_error(stream.str());
	}
	
	// bcas作成
	bcas = create_b_cas_card();
	if (bcas == NULL) throw b25_error("failed to create b_cas_card.");
	
	// bcas->init
	stat = bcas->init(bcas);
	if (stat < 0) {
		std::ostringstream stream;
		stream << "bcas->init failed. code=" << stat;
		throw b25_error(stream.str());
	}
	
	// b25->set_b_cas_card
	stat = b25->set_b_cas_card(b25, bcas);
	if (stat < 0) {
		std::ostringstream stream;
		stream << "b25->set_b_cas_card failed. code=" << stat;
		throw b25_error(stream.str());
	}
}

/**
 * デコーダへ入力ストリームの完了を伝える。
 * @exception b25_error エラー
 */
void
B25Decoder::flush() throw (b25_error)
{
	int stat = b25->flush(b25);
	if (stat < 0) {
		std::ostringstream stream;
		stream << "b25->flush failed. code=" << stat;
		throw b25_error(stream.str());
	}
}

/**
 * デコーダへストリームを入力する。
 * @param buf データ
 * @param len データの長さ
 * @exception b25_error エラー
 */
void
B25Decoder::put(const uint8_t *buf, int32_t len) throw (b25_error)
{
	// 内部でmemcpyされることを期待してconstを外す。
	ARIB_STD_B25_BUFFER sbuf = { (uint8_t *)buf, len };
	
	int stat = b25->put(b25, &sbuf);
	if (stat < 0) {
		std::ostringstream stream;
		stream << "b25->put failed. code=" << stat;
		throw b25_error(stream.str());
	}
}

/**
 * デコーダからストリームを取得する。
 * @param bufp データを入れるポインタへのポインタ
 * @return データの長さ
 * @exception b25_error エラー
 */
int32_t
B25Decoder::get(const uint8_t **bufp) throw (b25_error)
{
	ARIB_STD_B25_BUFFER dbuf = { NULL, 0 };
	int stat = b25->get(b25, &dbuf);
	if (stat < 0) {
		std::ostringstream stream;
		stream << "b25->get failed. code=" << stat;
		throw b25_error(stream.str());
	}
	
	*bufp = dbuf.data;
	return dbuf.size;
}
