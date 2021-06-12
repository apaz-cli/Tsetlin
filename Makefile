# A very basic makefile.

BINARY_NAME = a.out

CC = clang

SPEEDFLAGS = -Ofast -march=native -fwhole-program -flto 
DEBUGFLAGS = -Og -g -fsanitize=address
WARNFLAGS  = -Wall -Wextra -Wpedantic

OBJ = Main.o

DISASSEMBLY_NAME = bin.dmp

.PHONY: clean $(BINARY_NAME)

build: $(OBJ)
	$(CC) -o $(BINARY_NAME) $(OBJ) $(WARNFLAGS) $(SPEEDFLAGS)

debug: $(OBJ)
	$(CC) -o $(BINARY_NAME) $(OBJ) $(WARNFLAGS) $(DEBUGFLAGS)

dump:
	objdump -D $(BINARY_NAME) > $(DISASSEMBLY_NAME)
	less $(DISASSEMBLY_NAME)

run:
	./$(BINARY_NAME)

clean:
	rm *.o $(BINARY_NAME) $(DISASSEMBLY_NAME)