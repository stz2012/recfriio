# PC/SC Lite libraries and headers.
PCSC_CFLAGS ?= `pkg-config libpcsclite --cflags`
PCSC_LDLIBS ?= `pkg-config libpcsclite --libs`

CC     = gcc
CFLAGS = -Wall -O2 -g $(PCSC_CFLAGS) -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
LDLIBS = $(PCSC_LDLIBS) -lm
 
TARGET = b25
OBJS   = arib_std_b25.o b_cas_card.o multi2.o td.o ts_section_parser.o

all: $(TARGET)

arib_std_b25.o: arib_std_b25.c arib_std_b25.h portable.h b_cas_card.h arib_std_b25_error_code.h multi2.h ts_section_parser.h ts_common_types.h
b_cas_card.o: b_cas_card.c b_cas_card.h portable.h b_cas_card_error_code.h 
multi2.o: multi2.c multi2.h portable.h multi2_error_code.h 
td.o: td.c arib_std_b25.h portable.h b_cas_card.h 
ts_section_parser.o: ts_section_parser.c ts_section_parser.h ts_common_types.h portable.h ts_section_parser_error_code.h

$(TARGET): $(OBJS)
	$(CC) $(LDLIBS) -o $(TARGET) $(OBJS)

clean:
	rm -f *.o
	rm -f $(TARGET)
