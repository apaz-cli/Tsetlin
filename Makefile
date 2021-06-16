# A very basic makefile.

CC = clang++

SPEEDFLAGS = -Ofast -march=native -fwhole-program -flto
DEBUGFLAGS = -Og -g -fsanitize=address
WARNFLAGS  = -Wall -Wextra -Wshadow -Wpedantic

OBJ = Main.o

BITNUM = $(shell getconf LONG_BIT)

BINARY_NAME      = a.out
DISASSEMBLY_NAME = bin.dmp



.PHONY: clean $(BINARY_NAME)

build: $(OBJ)
	$(CC) -o $(BINARY_NAME) $(OBJ) -DBITNUM=$(BITNUM) $(WARNFLAGS) $(SPEEDFLAGS)

debug: $(OBJ)
	$(CC) -o $(BINARY_NAME) $(OBJ) -DBITNUM=$(BITNUM) $(WARNFLAGS) $(DEBUGFLAGS)

strip:
	strip -sx $(BINARY_NAME)

dump:
	objdump -D $(BINARY_NAME) > $(DISASSEMBLY_NAME)
	less $(DISASSEMBLY_NAME)

run:
	./$(BINARY_NAME)

clean:
	rm *.o $(BINARY_NAME) $(DISASSEMBLY_NAME)
