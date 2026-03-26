CC = gcc
CFLAGS = -Wall -Werror -g

SRC_DIR_TP1 = tp1
SRC_DIR_TP2 = tp2
OBJ_DIR = objects
BIN_DIR = target

OBJS = $(OBJ_DIR)/biceps.o $(OBJ_DIR)/gescom.o $(OBJ_DIR)/servcreme.o $(OBJ_DIR)/clicreme.o

all: $(BIN_DIR)/biceps

$(BIN_DIR)/biceps: $(OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^ -lreadline

# Règle pour les fichiers du TP1
$(OBJ_DIR)/%.o: $(SRC_DIR_TP1)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -I$(SRC_DIR_TP1) -c $< -o $@

# Règle pour le serveur
$(OBJ_DIR)/servcreme.o: $(SRC_DIR_TP2)/servcreme.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -I$(SRC_DIR_TP1) -c $< -o $@

# Règle pour le client
$(OBJ_DIR)/clicreme.o: $(SRC_DIR_TP2)/clicreme.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -I$(SRC_DIR_TP1) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

.PHONY: all clean