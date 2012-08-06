TARGETS = as11loader
CFLAGS = -O2 -g -lusb

$(TARGETS) : $(TARGETS).c
	gcc $(CFLAGS) -o $@ $^

all: $(TARGETS)

clean : 
	rm -f $(TARGETS)
