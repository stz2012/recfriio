// $Id$
// exceptions

#ifndef _ERROR_HPP_
#define _ERROR_HPP_

#include <string>
#include <exception>
#include <stdexcept>

/**
 * errnoをstd::stringに変換する。
 * @param e errno
 * @return std::string
 */
std::string stringError(int e);

/**
 * error
 */
class traceable_error : public std::runtime_error {
public:
	/**
	 * コンストラクタ
	 * @param __arg エラーメッセージ
	 */
	traceable_error(const std::string&  __arg)
		: std::runtime_error(__arg) {};
	
	/** デストラクタ */
	~traceable_error() throw () {};
	
	/**
	 * エラーメッセージ表示
	 */
	const char* whatNoTrace() const throw() {
		return what();
	};
};

/**
 * USBのエラー
 */
class usb_error : public traceable_error {
public:
	/**
	 * コンストラクタ
	 * @param __arg エラーメッセージ
	 */
	usb_error(const std::string&  __arg) : traceable_error(__arg) {};
};

/**
 * BUSY
 */
class busy_error : public traceable_error {
public:
	/**
	 * コンストラクタ
	 * @param __arg エラーメッセージ
	 */
	busy_error(const std::string&  __arg) : traceable_error(__arg) {};
};

/**
 * 初期化されていない場合
 */
class not_ready_error : public traceable_error {
public:
	/**
	 * コンストラクタ
	 * @param __arg エラーメッセージ
	 */
	not_ready_error(const std::string&  __arg) : traceable_error(__arg) {};
};

/**
 * file I/O エラー
 */
class io_error : public traceable_error {
public:
	/**
	 * コンストラクタ
	 * @param __arg エラーメッセージ
	 */
	io_error(const std::string&  __arg) : traceable_error(__arg) {};
};

#endif /* !defined(_ERROR_HPP_) */
