CC      ?= gcc
CFLAGS  ?= -Wall -Wextra
LDFLAGS ?= -lpthread

BISON   ?= bison
FLEX    ?= flex

SRC_DIR  = src
SRCS     = $(SRC_DIR)/main.c $(SRC_DIR)/parser.c $(SRC_DIR)/lexer.c
TARGET   = axt

.PHONY: all test clean format check-format lint coverage

all: $(TARGET)

$(SRC_DIR)/parser.c $(SRC_DIR)/parser.h: $(SRC_DIR)/parser.y
	$(BISON) -d -o $(SRC_DIR)/parser.c $(SRC_DIR)/parser.y

$(SRC_DIR)/lexer.c: $(SRC_DIR)/lexer.l $(SRC_DIR)/parser.h
	$(FLEX) -o $(SRC_DIR)/lexer.c $(SRC_DIR)/lexer.l

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $(SRCS) $(LDFLAGS)

test: $(TARGET)
	@sh test/run_tests.sh

clean:
	rm -f $(TARGET) $(SRC_DIR)/parser.c $(SRC_DIR)/parser.h $(SRC_DIR)/lexer.c *.o

format:
	@echo "format: not yet implemented"

check-format:
	@echo "check-format: not yet implemented"

lint:
	@echo "lint: not yet implemented"

coverage:
	@echo "coverage: not yet implemented"
