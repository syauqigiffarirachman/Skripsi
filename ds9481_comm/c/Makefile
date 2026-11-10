# Build dengan MinGW-w64 gcc (MSYS2):   mingw32-make      lalu   mingw32-make test
CC      ?= gcc
CFLAGS  ?= -O2 -Wall -Wextra -std=gnu99
SRC      = src

LIB_OBJS  = $(SRC)/serial_win32.o $(SRC)/crc.o $(SRC)/onewire.o $(SRC)/ds28e38.o
CRYPTO    = $(SRC)/sha256.o $(SRC)/p256.o

all: sign.exe

sign.exe: $(SRC)/sign.o $(LIB_OBJS) $(CRYPTO)
	$(CC) $(CFLAGS) -o $@ $^ -lshell32

tests/test_vectors.exe: tests/test_vectors.c $(SRC)/crc.o $(CRYPTO)
	$(CC) $(CFLAGS) -o $@ $^

test: tests/test_vectors.exe
	./tests/test_vectors.exe

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(SRC)/*.o sign.exe tests/*.exe

.PHONY: all test clean
