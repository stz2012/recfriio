TARGETS = as11loader
CFLAGS = -O2 -g
LIBS = -lusb

$(TARGETS) : $(TARGETS).c
	gcc $(CFLAGS) -o $@ $^ $(LIBS)

all: $(TARGETS)

clean : 
	rm -f $(TARGETS)
