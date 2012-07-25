#!/usr/bin/make -f
# $Id: Makefile 5663 2008-09-15 17:53:59Z clworld $

# options
B25      = -DB25
HDUS     = -DHDUS
UDP      = -DUDP

ifdef B25
  B25_PATH = ../arib25v025/arib25/src
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

DIST     = .
CC       = gcc
CXX      = g++
CXXFLAGS = -O2 -g -Wall -pthread -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 $(B25) $(HDUS) $(UDP) `pkg-config --cflags glib-2.0`
LDFLAGS  = 

OBJS   = recfriio.o usbops.o error.o IoThread.o Recordable.o AbstractFriio.o FriioWhite.o FriioBlack.o FriioWhiteWrapper.o FriioExpress.o tssplitter_lite.o $(B25_OBJS) $(HDUS_OBJS) $(UDP_OBJS)
LIBS   = -lpthread -lboost_thread-mt -lboost_filesystem `pkg-config --libs glib-2.0`
TARGET = $(DIST)/recfriio
DEPEND = Makefile.dep

all: $(TARGET)

clean: $(B25_CLEAN)
	rm -f $(OBJS) $(TARGET) $(DEPEND)

ifdef B25
clean_b25:
	cd $(B25_PATH); make clean
endif

$(TARGET):	$(OBJS) $(B25_OBJS_EXT)
	$(CXX) $(LDFLAGS) -o $(TARGET) $(OBJS) $(B25_OBJS_EXT) $(LIBS) $(B25_LIBS)

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
