# Compiler and Flags
CC = gcc
CFLAGS = -Wall -Wextra -g -Iinclude
LDFLAGS =

# Source and Object Files
SRCS = src/lexer.c src/parser.c src/codegen.c src/main.c
OBJS = $(SRCS:.c=.o)

# Target Executable
TARGET = minicc

# Default Goal
.DEFAULT_GOAL = all

# --- Targets ---

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# Pattern rule for object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
	rm -rf tests/output/* # Clean test outputs

test: all
	@echo "Running tests..."
	@./tests/run_tests.sh

# Phony targets to prevent conflicts with files named 'all', 'clean', 'test'
.PHONY: all clean test
