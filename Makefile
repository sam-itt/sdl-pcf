SRCDIR=.

CC=gcc
CFLAGS=-g3 -O0 `pkg-config sdl2 --cflags` -I$(SRCDIR)
LDFLAGS=-lz -lm `pkg-config sdl2 --libs` -Wl,--as-needed
EXEC=test-pcf
SRC= $(wildcard $(SRCDIR)/*.c)
OBJ= $(SRC:.c=.o)

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS)

.PHONY: clean mrproper

clean:
	rm -rf *.o

mrproper: clean
	rm -rf $(EXEC)

