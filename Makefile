CC      ?= gcc
CFLAGS  ?= -Wall -Wextra
LDFLAGS ?= -lpthread

BISON   ?= bison
FLEX    ?= flex

SRC_DIR  = src
SRCS     = $(SRC_DIR)/main.c $(SRC_DIR)/ast.c $(SRC_DIR)/varexpand.c $(SRC_DIR)/envfile.c $(SRC_DIR)/os_compat.c $(SRC_DIR)/runner.c $(SRC_DIR)/threadpool.c $(SRC_DIR)/progress.c $(SRC_DIR)/parser.c $(SRC_DIR)/lexer.c
TARGET   = axt

.PHONY: all test clean format check-format lint coverage amalgamate

all: $(TARGET) axt.c

$(SRC_DIR)/parser.c $(SRC_DIR)/parser.h: $(SRC_DIR)/parser.y
	$(BISON) -d -o $(SRC_DIR)/parser.c $(SRC_DIR)/parser.y

$(SRC_DIR)/lexer.c: $(SRC_DIR)/lexer.l $(SRC_DIR)/parser.h
	$(FLEX) -o $(SRC_DIR)/lexer.c $(SRC_DIR)/lexer.l

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $(SRCS) $(LDFLAGS)

test: $(TARGET)
	AXT=$$(pwd)/axt ./axt -j 1 test/test_suite.at

clean:
	rm -f $(TARGET) axt.c $(SRC_DIR)/parser.c $(SRC_DIR)/parser.h $(SRC_DIR)/lexer.c *.o

FORMAT_SRCS = $(SRC_DIR)/main.c $(SRC_DIR)/ast.h $(SRC_DIR)/ast.c \
              $(SRC_DIR)/varexpand.h $(SRC_DIR)/varexpand.c \
              $(SRC_DIR)/envfile.h $(SRC_DIR)/envfile.c \
              $(SRC_DIR)/os_compat.h $(SRC_DIR)/os_compat.c \
              $(SRC_DIR)/runner.h $(SRC_DIR)/runner.c \
              $(SRC_DIR)/threadpool.h $(SRC_DIR)/threadpool.c \
              $(SRC_DIR)/progress.h $(SRC_DIR)/progress.c

format:
	clang-format -i $(FORMAT_SRCS)

check-format:
	clang-format --dry-run --Werror $(FORMAT_SRCS)

lint:
	cppcheck --enable=all --error-exitcode=1 --suppress=missingIncludeSystem --suppress=unusedFunction $(FORMAT_SRCS)

HAND_SRCS = $(SRC_DIR)/main.c $(SRC_DIR)/ast.h $(SRC_DIR)/ast.c \
            $(SRC_DIR)/varexpand.h $(SRC_DIR)/varexpand.c \
            $(SRC_DIR)/envfile.h $(SRC_DIR)/envfile.c \
            $(SRC_DIR)/os_compat.h $(SRC_DIR)/os_compat.c \
            $(SRC_DIR)/runner.h $(SRC_DIR)/runner.c \
            $(SRC_DIR)/threadpool.h $(SRC_DIR)/threadpool.c \
            $(SRC_DIR)/progress.h $(SRC_DIR)/progress.c \
            $(SRC_DIR)/parser.y $(SRC_DIR)/lexer.l

axt.c: $(HAND_SRCS)
	sh amalgamate.sh

amalgamate: axt.c

coverage:
	@echo "coverage: not yet implemented"
