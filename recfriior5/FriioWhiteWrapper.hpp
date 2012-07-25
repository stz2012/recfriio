// $Id$
// 白凡のラッパ

#ifndef _FRIIO_WHITE_WRAPPER_H_
#define _FRIIO_WHITE_WRAPPER_H_

#include "Recordable.hpp"
#include "AbstractFriio.hpp"

class FriioWhiteWrapper : public AbstractFriio
{
public:
  FriioWhiteWrapper();

  // 全部委譲なのだよ明智くん
  ~FriioWhiteWrapper() {
	if(tuner != NULL) {
	  delete tuner;
	  // そうか親のデコンストラクタって常に呼び出されるのか
	  initialized = false;
	}
  }

  virtual void setChannel(BandType band, int channel) {
    if(tuner != NULL) {
      tuner->setChannel(band, channel);
    }
  }
  
  virtual const float getSignalLevel(void) {
	if(tuner != NULL) {
	  return tuner->getSignalLevel();
	}
	
	return 0.0f;
  }
  
  virtual void startStream() {
	if(tuner != NULL) {
	  tuner->startStream();
	}
  }
  
  virtual void stopStream() {
	if(tuner != NULL) {
	  tuner->stopStream();
	}
  }
  
  virtual int getStream(const uint8_t** bufp, int timeoutMsec) {
	if(tuner != NULL) {
	  return tuner->getStream(bufp, timeoutMsec);
	}
	return -1;
  }
  
  virtual bool isStreamReady(){
	if(tuner != NULL) {
	  return tuner->isStreamReady();
	}
	return false;
  }
  
  virtual void clearBuffer() {
	if(tuner != NULL) {
	  tuner->clearBuffer();
	}
  }
  
protected:
  virtual void UsbInitalize(int fd, bool lnb) {
	if(tuner != NULL) {
	  return tuner->UsbInitalize(fd, lnb);
	}
  }

  virtual void UsbTerminate(int fd) {
	if(tuner != NULL) {
	  return tuner->UsbTerminate(fd);
	}
  }
	
  virtual void UsbProcBegin(int fd) {
	if(tuner != NULL) {
	  return tuner->UsbProcBegin(fd);
	}
  }
  
  virtual void UsbProcEnd(int fd) {
	if(tuner != NULL) {
	  return tuner->UsbProcEnd(fd);
	}
  }
  
  virtual void UsbProcLED(int fd, BonLedColor color, bool lnb_powered) {
	if(tuner != NULL) {
	  return tuner->UsbProcLED(fd, color, lnb_powered);
	}
  }
  
  virtual void UsbProcFixInitB(int fd)  {
	if(tuner != NULL) {
	  return tuner->UsbProcFixInitB(fd);
	}
  }
  

  /**
   * これだけ実装があります
   */
  virtual TunerType UsbProcFixInitA(int fd);

private:
  AbstractFriio *tuner;
  void copyinstance(int fd);
};

#endif /* !defined(_FRIIO_WHITE_H_) */
