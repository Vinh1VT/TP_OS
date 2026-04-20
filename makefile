CC = gcc
CFLAGS = -Wall -Werror -g

SRC_DIR_TP1 = tp1
SRC_DIR_TP3 = tp3
OBJ_DIR = objects
BIN_DIR = target

# On utilise maintenant servtp3.o au lieu des fichiers du TP2
OBJS = $(OBJ_DIR)/biceps.o $(OBJ_DIR)/gescom.o $(OBJ_DIR)/servtp3.o

all: $(BIN_DIR)/biceps

$(BIN_DIR)/biceps: $(OBJS) | $(BIN_DIR)
# Ajout de -lpthread ici pour compiler la librairie des threads
	$(CC) $(CFLAGS) -o $@ $^ -lreadline -lpthread

# Règle pour les fichiers du TP1
$(OBJ_DIR)/%.o: $(SRC_DIR_TP1)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -I$(SRC_DIR_TP1) -c $< -o $@

# Règle pour le nouveau serveur TP3
$(OBJ_DIR)/servtp3.o: $(SRC_DIR_TP3)/servtp3.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -I$(SRC_DIR_TP3) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

.PHONY: all clean