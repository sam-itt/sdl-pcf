SRCDIR=.

CC=gcc
CFLAGS=-g3 -O0 `pkg-config sdl2 --cflags` -I$(SRCDIR) -DHAVE_SDL2
LDFLAGS=-lz -lm `pkg-config sdl2 --libs` -Wl,--as-needed
SRC= $(filter-out $(SRCDIR)/test.c $(SRCDIR)/test-static.c, $(wildcard $(SRCDIR)/*.c))
OBJ= $(SRC:.c=.o)

all: test-pcf test-static

test-pcf: $(OBJ) test.o
	$(CC) -o $@ $^ $(LDFLAGS)

test-static: $(OBJ) test-static.o
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS)

.PHONY: clean mrproper

clean:
	rm -rf *.o

mrproper: clean
	rm -rf $(EXEC)

