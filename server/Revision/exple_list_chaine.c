#include <stdio.h>
#include <stdlib.h>

// 1. Définition du "Wagon" (Le Noeud)
struct Element {
    int nombre;             // La donnée mathématique
    struct Element *suivant; // Le lien vers le prochain
};

int main() {
    // 2. Création de 3 wagons manuellement
    struct Element *un = malloc(sizeof(struct Element));
    struct Element *deux = malloc(sizeof(struct Element));
    struct Element *trois = malloc(sizeof(struct Element));

    // 3. On leur donne des valeurs
    un->nombre = 10;
    deux->nombre = 20;
    trois->nombre = 30;

    // 4. On les lie entre eux (La Chaîne)
    un->suivant = deux;   // 10 est lié à 20
    deux->suivant = trois; // 20 est lié à 30
    trois->suivant = NULL; // 30 est le dernier (fin de liste)

    // 5. Opération mathématique : Calculer la somme en parcourant la liste
    int somme = 0;
    struct Element *actuel = un; // On commence par le premier

    while (actuel != NULL) {
        somme += actuel->nombre; // On ajoute la valeur
        actuel = actuel->suivant; // On passe au lien suivant
    }

    printf("La somme totale est : %d\n", somme); // Affiche 60

    // Nettoyage (important)
    free(un); free(deux); free(trois);
    return 0;
}
