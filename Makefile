# A very basic makefile.

CC = clang++

NORMFLAGS = --std=c++20
SPEEDFLAGS = -Ofast -march=native -fwhole-program -flto
DEBUGFLAGS = -Og -g -fsanitize=address
WARNFLAGS  = -Wall -Wextra -Wshadow -Wpedantic -Wimplicit

OBJ = Main.o

BITNUM = $(shell getconf LONG_BIT)

BINARY_NAME      = a.out
DISASSEMBLY_NAME = bin.dmp



.PHONY: clean $(BINARY_NAME)

build: $(OBJ)
	$(CC) -o $(BINARY_NAME) $(OBJ) -DBITNUM=$(BITNUM) $(NORMFLAGS) $(WARNFLAGS) $(SPEEDFLAGS)

debug: $(OBJ)
	$(CC) -o $(BINARY_NAME) $(OBJ) -DBITNUM=$(BITNUM) $(NORMFLAGS) $(WARNFLAGS) $(DEBUGFLAGS)

strip:
	strip -sx $(BINARY_NAME)

dump:
	objdump -D $(BINARY_NAME) > $(DISASSEMBLY_NAME)
	less $(DISASSEMBLY_NAME)

run:
	./$(BINARY_NAME)

clean:
	rm *.o $(BINARY_NAME) $(DISASSEMBLY_NAME)
