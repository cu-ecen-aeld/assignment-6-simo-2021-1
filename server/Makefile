# Makefile for aesdsocket
# 07JAN2026 - Arnaud SImo

# Name of the Exe
TARGET = aesdsocket
 
# Sources (ajoutez d'autres fichiers .c ici si besoin)
SRC = aesdsocket.c
OBJ = $(SRC:.c=.o)  # Génère les noms de fichiers objets automatiquement

# Ne PAS définir CC : Buildroot le fournit
# Ne PAS définir CROSS_COMPILE

# Ajouter nos flags sans écraser ceux de Buildroot
# -g est ajouté ici (plutôt que dans la règle) pour que Buildroot le prenne en compte
CFLAGS += -Wall -g
# Désactiver les optimisations en mode debug (facultatif mais mieux pour GDB)
# Si Buildroot utilise -O2, ce flag le remplace pour le débogage
CFLAGS_DEBUG = -O0 -g

# Règle par défaut
all: $(TARGET)

# Construction du binaire (utilise les objets pour la compilation incrémentale)
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

# Compilation des fichiers .c en .o (règle implicite améliorée)
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Règle pour build en mode debug (sans optimisations)
debug: CFLAGS += $(CFLAGS_DEBUG)
debug: clean $(TARGET)

# Nettoyage (supprime aussi les fichiers .o générés)
clean:
	rm -f $(TARGET) $(OBJ)

distclean: clean
	# Supprime les fichiers générés par Buildroot (si besoin)
	rm -f .*.depend *.d

# Gestion des dépendances (évite de recompiler tout si un .h change)
-include $(OBJ:.o=.d)
%.d: %.c
	$(CC) $(CFLAGS) -MM -MT $@ -MT $(@:.d=.o) $< -o $@
