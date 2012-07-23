// $Id$
// Thread間データ転送用 リングバッファ(FIFO)管理クラス

#ifndef _RINGBUF_HPP_
#define _RINGBUF_HPP_

#include <iostream>
#include <exception>
#include <stdexcept>
#include <boost/thread.hpp>
#include <boost/thread/condition.hpp>

// バッファのオーバーフロー時に投げられます。
class overflow_error : public std::runtime_error {
public:
	overflow_error(const std::string&  __arg) : std::runtime_error(__arg) {
	};
};

// 割り込まれた時に投げられます。
class interrupted_error : public std::runtime_error {
public:
	interrupted_error(const std::string&  __arg) : std::runtime_error(__arg) {
	};
};

// cppは素人なのでコピーとか全く考えていません。
template <class T> class RingBuf {
public:
	// コンストラクタ/デストラクタ
	RingBuf(size_t size);
	virtual ~RingBuf();
	
	// バッファオーバーフローが起こっている。
	bool isOverflow();
	
	// 書込み中Node数
	unsigned int getWaitCount();
	
	// 読込み可能Node数
	unsigned int getReadyCount();
	
	// 次の書込み側のポインタを取得する。
	// timeout_msec以上setReadyされなかった場合 timeout_handler(T*) が呼ばれる。
	// Timeout処理はバッファからの読み出し時に先頭の要素に対して行われる。
	T *getPushPtr(unsigned int timeout_msec, void (*timeout_handler)(T *));
	
	// 書込み完了通知
	void setReady(T *node);
	
	// 次の読込み側ポインタを取得する。
	const T *getPopPtr();
	
	// 次の読込み側ポインタを取得する(タイムアウト付)
	// timeout時NULL
	const T *getPopPtr(unsigned int timeout_msec);
	
	// 次の読込み側ポインタを取得する(ブロックしない)
	// 現在読込めない場合はNULL
	const T *peekPopPtr();
	
	// 入力待ち強制終了用 割り込む。
	void interrupt();
	
protected:
	
	// peekPopPtrのmutexをロックしない版
	const T *peekPopPtrWithoutLock();
	
	// xtime を msec ミリ秒後の値にする。
	void xtimeAfterMsec(boost::xtime *xtime, unsigned int msec);
	
	// リングバッファのノード
	struct Node {
		T data;     // データ本体
		Node *next; // 次
		boost::xtime timeout; // タイムアウト時刻
		void (*timeout_handler)(T *); // タイムアウトハンドラ
		bool is_wait;  // 書込み中?
		bool is_ready; // 読込み可能か？
	};
	
	bool is_overflow;
	bool is_interrupted;
	
	Node *node_top;
	Node *node_push;
	Node *node_pop;
	
	boost::mutex     mutex;
	boost::condition cond;

	unsigned int wait_count;
	unsigned int ready_count;
};

template <class T>
RingBuf<T>::RingBuf(size_t size) {
	is_overflow = false;
	is_interrupted = false;
	
	node_top = node_push = node_pop = new Node[size];
	wait_count = ready_count = 0;
	
	// リングバッファ設定
	for (uint32_t i = 0; i < size; i++) {
		node_top[i].next = (i+1 == size) ? &(node_top[0]) : &(node_top[i+1]);
		boost::xtime_get(&(node_top[i].timeout), boost::TIME_UTC);
		node_top[i].timeout_handler = NULL;
		node_top[i].is_ready = false;
		node_top[i].is_wait  = false;
	}
}
template <class T>
RingBuf<T>::~RingBuf() {
	delete[] node_top;
}

// バッファオーバーフローが起こっている。
template <class T>
bool
RingBuf<T>::isOverflow() {
	boost::mutex::scoped_lock lock(mutex);
	return is_overflow;
}

// 書込み中Node数
template <class T>
unsigned int 
RingBuf<T>::getWaitCount() {
	boost::mutex::scoped_lock lock(mutex);
	return wait_count - ready_count;
}

// 読込み可能Node数
template <class T>
unsigned int 
RingBuf<T>::getReadyCount() {
	boost::mutex::scoped_lock lock(mutex);
	return ready_count;
}

// 次の書込み側のポインタを取得する。
template <class T>
T *
RingBuf<T>::getPushPtr(unsigned int timeout_msec, void (*timeout_handler)(T *)) {
	boost::mutex::scoped_lock lock(mutex);
	if (is_overflow) {
		throw overflow_error("RingBuf was overflowed.");
	}
	
	// Overflowチェック
	if (node_push->next == node_pop || node_push->next->next == node_pop) {
		is_overflow = true;
		throw overflow_error("RingBuf is overflow.");
	}
	
	T *data = &(node_push->data);
	boost::xtime_get(&(node_push->timeout), boost::TIME_UTC);
	xtimeAfterMsec(&(node_push->timeout), timeout_msec);
	node_push->timeout_handler = timeout_handler;
	node_push->is_wait = true;
	wait_count++;
	
	node_push = node_push->next;
	
	return data;
	
}

// 書込み完了通知
template <class T>
void
RingBuf<T>::setReady(T *node) {
	boost::mutex::scoped_lock lock(mutex);
	if (is_overflow) {
		throw overflow_error("RingBuf was overflowed.");
	}
	
	Node *real_node = reinterpret_cast<Node *>(node);
	if (real_node->is_wait) {
		real_node->is_ready = true;
		ready_count++;

	} else {
		// TODO: なぜかTimeoutしたURBをキャンセルするとここにくる。
		// std::cerr << "SetReady called for not waiting node." << std::endl;
	}
	cond.notify_one();
}

// 次の読込み側ポインタを取得する。
template <class T>
const T *
RingBuf<T>::getPopPtr() {
	const T *data = NULL;
	while(true) {
		data = getPopPtr(100);
		if (NULL != data) {
			break; // read.
		}
	}
	
	return data;
}

// 次の読込み側ポインタを取得する(タイムアウト付)
// timeout時NULL
template <class T>
const T *
RingBuf<T>::getPopPtr(unsigned int timeout_msec) {
	boost::mutex::scoped_lock lock(mutex);
	if (is_overflow) {
		throw overflow_error("RingBuf was overflowed.");
	}
	
	boost::xtime wait_until;
	const T *data = NULL;
	while(true) {
		if (is_interrupted) {
			throw interrupted_error("inturrupted!");
		}
		
		data = peekPopPtrWithoutLock();
		if (NULL != data) {
			break; // read.
		}
		
		// Timeout時間の厳密性を考えるのが面倒なので適当に処理する。
		boost::xtime_get(&wait_until, boost::TIME_UTC);
		xtimeAfterMsec(&wait_until, timeout_msec);

		// timeout_msecで待つ
		if (!cond.timed_wait(lock, wait_until)) {
			return NULL; // Timeout;
		}
	}
	
	return data;
}

// 次の読込み側ポインタを取得する(ブロックしない)
// 現在読込めない場合はNULL
template <class T>
const T *
RingBuf<T>::peekPopPtr() {
	boost::mutex::scoped_lock lock(mutex);
	if (is_overflow) {
		throw overflow_error("RingBuf was overflowed.");
	}
	
	return peekPopPtrWithoutLock();
}

// 割り込む
template <class T>
void
RingBuf<T>::interrupt() {
	boost::mutex::scoped_lock lock(mutex);	
	is_interrupted = true;
	cond.notify_one();
}

// 既にロックされている状態で読込めるか確認
template <class T>
const T *
RingBuf<T>::peekPopPtrWithoutLock() {
	if (is_overflow) {
		throw overflow_error("RingBuf was overflowed.");
	}
	if (node_pop != node_push) {
		if (node_pop->is_ready) {
			// リセットして返す。
			boost::xtime_get(&(node_pop->timeout), boost::TIME_UTC);
			node_pop->timeout_handler = NULL;
			node_pop->is_ready = false;
			node_pop->is_wait  = false;
			const T *data = &(node_pop->data);
			
			node_pop = node_pop->next;
			ready_count--;
			wait_count--;
			
			return data;
		}
		
		boost::xtime now;
		boost::xtime_get(&now, boost::TIME_UTC);
		if (boost::xtime_cmp(node_pop->timeout, now) <= 0) {
			// Timeoutを過ぎている。
			if (node_pop->timeout_handler) {
				// timeout_handler呼出し
				node_pop->timeout_handler(&(node_pop->data));
			}
			
			// リセットして次へ。
			boost::xtime_get(&(node_pop->timeout), boost::TIME_UTC);
			node_pop->timeout_handler = NULL;
			node_pop->is_ready = false;
			node_pop->is_wait  = false;
			
			node_pop = node_pop->next;
			wait_count--;
			
			return peekPopPtrWithoutLock();
		}
	}
	
	return NULL;
}

// xtime を msec ミリ秒後の値にする。
template <class T>
void
RingBuf<T>::xtimeAfterMsec(boost::xtime *xtime, unsigned int msec) {
	const uint64_t NANOSECONDS_PER_SECOND = 1000*1000*1000;
	
	uint64_t nsec = (uint64_t)xtime->nsec + (uint64_t)(msec % 1000) * 1000 * 1000;
	uint64_t sec  = nsec / NANOSECONDS_PER_SECOND;
	nsec -= sec * NANOSECONDS_PER_SECOND;
	
	xtime->sec  += sec + msec / 1000;
	xtime->nsec  = nsec;
}

#endif
