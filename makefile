CC = gcc
CFLAGS = -std=gnu11 -Wall -Wextra -Werror -g

SRC_DIR_TP1 = tp1
SRC_DIR_TP3 = tp3
OBJ_DIR = objects

# On compile avec servtp3.c
OBJS = $(OBJ_DIR)/biceps.o $(OBJ_DIR)/gescom.o $(OBJ_DIR)/servtp3.o

all: biceps

# Cible principale (biceps à la racine)
biceps: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ -lreadline -lpthread

# Cible pour Valgrind (désactive les optimisations avec -O0)
memory-leak: CFLAGS = -Wall -Werror -g -O0
memory-leak: $(OBJS)
	$(CC) $(CFLAGS) -o biceps $^ -lreadline -lpthread

# Règles de compilation des objets
$(OBJ_DIR)/%.o: $(SRC_DIR_TP1)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -I$(SRC_DIR_TP1) -c $< -o $@

$(OBJ_DIR)/servtp3.o: $(SRC_DIR_TP3)/servtp3.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -I$(SRC_DIR_TP3) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# Nettoie tout, y compris les exécutables à la racine
clean:
	rm -rf $(OBJ_DIR) biceps biceps-memory-leaks

.PHONY: all clean memory-leak