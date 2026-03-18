CC = gcc
CFLAGS = -Wall -Wextra -g

SRC_DIR = tp1
OBJ_DIR = objects
BIN_DIR = target

OBJS = $(OBJ_DIR)/biceps.o $(OBJ_DIR)/gescom.o

all: $(BIN_DIR)/biceps

$(BIN_DIR)/biceps: $(OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^ -lreadline

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -I$(SRC_DIR) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

.PHONY: all clean