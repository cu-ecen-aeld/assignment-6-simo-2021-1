# Définir un préfixe pour cross-compiler
CROSS_COMPILE ?= #aarch64-linux-gnu-

# Nom de l'exécutable final
TARGET = writer

# Nom du fichier source
SRC = writer.c

# Compilateur utilisé
CC = $(CROSS_COMPILE)gcc

# Options de compilation :
# -Wall  : active les warnings (recommandé pour débutants)
# -g     : ajoute les infos de debug (utile pour gdb)
CFLAGS = -Wall -g

# Règle par défaut (celle exécutée si on tape juste "make")
all: $(TARGET)

# Règle pour créer l'exécutable
# $@ = nom de la cible (writer)
# $< = premier fichier dépendant (writer.c)
$(TARGET): $(SRC)
	#$(CC) $(CFLAGS) $< -o $@
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

# Nettoyage : supprime l'exécutable
clean:
	rm -f $(TARGET)

# Nettoyage total (ex: fichiers temporaires)
distclean: clean
	rm -f *.o
