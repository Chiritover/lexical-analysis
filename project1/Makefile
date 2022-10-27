TARGET := lexer

GCC = @g++
CXXFLAGS  := -g -Wall -O3 -std=c++17
LEX = @flex

BUILD_DIR := build
BIN_DIR := bin
SRC_DIR := src

LEX_IN :=src/lexer.lex
LEX_CPP := src/lexer.cpp

start: $(BIN_DIR)/$(TARGET)
	mkdir -p output
	$< $(INPUT)

$(BIN_DIR)/$(TARGET): $(LEX_CPP) src/main.cpp
			mkdir -p bin
			$(GCC) $(CXXFLAGS) -o $@ $^

$(LEX_CPP): $(LEX_IN)
			$(LEX) -o $(LEX_CPP) ${LEX_IN}

clean:
				@-rm -f lexer.c lexer
.PHONY: clean