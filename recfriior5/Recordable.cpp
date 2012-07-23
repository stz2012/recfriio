// $Id: Recordable.cpp 5663 2008-09-15 17:53:59Z clworld $
// 

#include "FriioWhite.hpp"
#include "FriioBlack.hpp"

const std::string tunerTypeName[TUNER_TYPES_LEN] = {
	"Friio(White)",
	"Friio(Black)",
};

extern "C" Recordable*
createRecordable(TunerType type) {
	switch(type) {
		case TUNER_FRIIO_WHITE:
			return new FriioWhite();
			break;
		case TUNER_FRIIO_BLACK:
			return new FriioBlack();
			break;
		default:
			return NULL;
	}
}
