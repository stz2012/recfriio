// $Id: error.cpp 5663 2008-09-15 17:53:59Z clworld $
// exception

#include <stdlib.h>
#include <string.h>

#include "error.hpp"

/**
 * errnoをstd::stringに変換する。
 * @param e errno
 * @return エラー内容
 */
std::string stringError(int e)
{
	char buf[1024];
	memset(buf, '\0', sizeof(buf));
	return std::string(strerror_r(e, buf, sizeof(buf)));
}
