CC := gcc
CFLAGS := -Wall 
LDLIBS := -l pthread
PROGS := server.out \ client.out

all: $(PROGS)

%.out: %.c
	$(CC) $(CFLAGS) $< $(LDLIBS) -o $@ 
       
       
.PHONY: all clean

clean:
	rm -f *.out
