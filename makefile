BIN=./bin/
SOURCE=./src/
CFLAGS=-fopenmp
LIST=$(BIN)/hello
CC=g++

all: $(LIST)

$(BIN)/%:  $(SOURCE)%.cpp
	$(CC) $(INC) $< $(CFLAGS) -o $@ $(LIBS)
