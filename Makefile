CC=cc

CFLAGS=-Wall -pedantic -std=c99
LDFLAGS=-lncurses

PROG=sniper
OBJS=\
	util.o\
	text.o\
	rune.o\
	main.o

all: clean $(PROG)

clean:
	rm -f *.o

$(PROG): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

.c.o:
	$(CC) -c $(CFLAGS) $<

# utf/
rune.o: utf/rune.c
	$(CC) -c $^ $(CFLAGS)
