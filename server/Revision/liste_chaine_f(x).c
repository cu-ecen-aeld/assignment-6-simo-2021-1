#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

// La structure du maillon (notre "index" mathématique)
typedef struct node {
    pthread_t thread_id;
    int resultat;
    struct node *next;
} thread_node_t;

// La fonction que chaque thread exécute : f(x) = x * x
void* calculer_carre(void* arg) {
    int x = *(int*)arg;
    int* res = malloc(sizeof(int));
    *res = x * x;
    free(arg); 
    return res; // On renvoie le résultat au join
}

int main() {
    thread_node_t *head = NULL;
    int somme_totale = 0;

    // 1. CRÉATION (Sigma de i=1 à 5)
    for (int i = 1; i <= 5; i++) {
        thread_node_t *nouveau = malloc(sizeof(thread_node_t));
        int *val = malloc(sizeof(int));
        *val = i;
        
        pthread_create(&nouveau->thread_id, NULL, calculer_carre, val);
        
        // Ajout dans la liste chaînée
        nouveau->next = head;
        head = nouveau;
    }

    // 2. SYNCHRONISATION (Le pthread_join)
    // On parcourt la liste pour récupérer chaque f(i)
    thread_node_t *courant = head;
    while (courant != NULL) {
        int *val_retour;
        
        // On attend que le thread i finisse pour récupérer son résultat
        pthread_join(courant->thread_id, (void**)&val_retour);
        
        somme_totale += *val_retour;
        
        // Nettoyage mémoire
        free(val_retour);
        thread_node_t *a_supprimer = courant;
        courant = courant->next;
        free(a_supprimer);
    }

    printf("Somme des carrés (1²+2²+3²+4²+5²) = %d\n", somme_totale);
    return 0;
}
