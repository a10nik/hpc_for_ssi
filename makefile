BIN=./bin/
SOURCE=./src/
CFLAGS=-fopenmp -std=c++14
LIST=$(BIN)/hello
CC=g++

all: $(LIST)

$(BIN)/%:  $(SOURCE)%.cpp
	$(CC) $(INC) $< $(CFLAGS) -o $@ $(LIBS)
