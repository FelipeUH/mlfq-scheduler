CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g -Isrc

SRC_DIR = src
TEST_DIR = tests
BUILD_DIR = build

SRCS = $(wildcard $(SRC_DIR)/*.c)
MAIN_OBJ = $(BUILD_DIR)/main.o
TEST_OBJ = $(BUILD_DIR)/test_main.o

# Excluir main.c de los tests
SRCS_NO_MAIN = $(filter-out $(SRC_DIR)/main.c, $(SRCS))

.PHONY: all clean test

all: mlfq

mlfq: $(SRCS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(SRCS)

test: $(SRCS_NO_MAIN) $(TEST_DIR)/test_mlfq.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $(BUILD_DIR)/test_mlfq $(SRCS_NO_MAIN) $(TEST_DIR)/test_mlfq.c
	./$(BUILD_DIR)/test_mlfq

clean:
	rm -rf $(BUILD_DIR) mlfq results.csv simulation.log
