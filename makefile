BIN=./bin/
SOURCE=./src/
CFLAGS=-fopenmp -std=c++14 -O3
CC=g++

all: $(BIN)/par_sort $(BIN)/matrix_mul

$(BIN)/%:  $(SOURCE)%.cpp
	$(CC) $(INC) $< $(CFLAGS) -o $@ $(LIBS)
