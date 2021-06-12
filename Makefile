# A very basic makefile.

BINARY_NAME = a.out

CC = clang

SPEEDFLAGS = -Ofast -march=native -fwhole-program -flto 
DEBUGFLAGS = -Og -g -fsanitize=address

OBJ = Main.o

.PHONY: clean $(BINARY_NAME)

build: $(OBJ)
	$(CC) -o $(BINARY_NAME) $(OBJ) $(SPEEDFLAGS)

debug: $(OBJ)
	$(CC) -o $(BINARY_NAME) $(OBJ) $(DEBUGFLAGS)

run:
	./$(BINARY_NAME)

clean:
	rm *.o $(BINARY_NAME)