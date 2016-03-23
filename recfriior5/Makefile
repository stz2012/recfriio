# options
B25      = -DB25
HDUS     = -DHDUS
UDP      = -DUDP
#HTTP     = -DHTTP
TSSL     = -DTSSL

ifdef B25
  B25_PATH = ../arib25
  B25_CLEAN = clean_b25
  B25_OBJS = B25Decoder.o
  B25_OBJS_EXT = $(B25_PATH)/arib_std_b25.o $(B25_PATH)/b_cas_card.o $(B25_PATH)/multi2.o $(B25_PATH)/ts_section_parser.o
  PCSC_LDLIBS ?= `pkg-config libpcsclite --libs`
  B25_LIBS = $(PCSC_LDLIBS) -lm
endif

ifdef HDUS
  HDUS_OBJS = Hdus.o Hdp.o
endif

ifdef UDP
  UDP_OBJS = Udp.o
endif

ifdef TSSL
  TSSL_OBJS = tssplitter_lite.o
endif

PREFIX = /usr/local
TARGET = recfriio
OBJS   = recfriio.o usbops.o error.o IoThread.o Recordable.o AbstractFriio.o FriioWhite.o FriioBlack.o FriioWhiteWrapper.o FriioExpress.o $(B25_OBJS) $(HDUS_OBJS) $(UDP_OBJS) $(TSSL_OBJS)
DEPEND = Makefile.dep

CC       = gcc
CXX      = g++
CXXFLAGS = -O2 -g -Wall -pthread -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 $(B25) $(HDUS) $(UDP) $(HTTP) $(TSSL) `pkg-config --cflags glib-2.0`
LDFLAGS  = 
LIBS     = -lpthread -lboost_thread -lboost_system -lboost_filesystem `pkg-config --libs glib-2.0`

all: $(TARGET)

clean: $(B25_CLEAN)
	rm -f $(OBJS) $(TARGET) $(DEPEND)

uninstall:
	rm -vf $(PREFIX)/bin/$(TARGET) /etc/udev/rules.d/99-friio.rules

install: uninstall
	if [ -d /etc/udev/rules.d -a ! -f /etc/udev/rules.d/99-friio.rules ] ; then \
		install -m 644 etc/99-friio.rules /etc/udev/rules.d ; \
	fi
	install -m 755 $(TARGET) $(PREFIX)/bin

ifdef B25
clean_b25:
	cd $(B25_PATH); make clean
endif

$(TARGET): $(OBJS) $(B25_OBJS_EXT)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) $(B25_OBJS_EXT) $(LIBS) $(B25_LIBS)

$(DEPEND):
	$(CC) -MM $(OBJS:.o=.cpp) > Makefile.dep

# B25_OBJS_EXT
$(B25_PATH)/arib_std_b25.o:
	cd $(B25_PATH); make all
$(B25_PATH)/b_cas_card.o:
	cd $(B25_PATH); make all
$(B25_PATH)/multi2.o:
	cd $(B25_PATH); make all
$(B25_PATH)/ts_section_parser.o:
	cd $(B25_PATH); make all

-include $(DEPEND)
