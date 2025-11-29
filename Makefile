TARGET = bin/jeu

CC = gcc
CFLAGS = -Wall
SRC = main.c

# Compilation de l'exécutable
$(TARGET): $(OBJS)
	$(CC) $(SRC) -o $(TARGET)

dev: clean $(TARGET)

# Supprimer fichier existant
clean:
	rm -f $(OBJS) $(TARGET)

# Build
build: clean $(TARGET)

# Lancer le jeu
run: $(TARGET)
	./$(TARGET)

# Lancer en mode dev
devrun: dev
	./$(TARGET)

verif:
	@bash scripts/verif.sh main.c