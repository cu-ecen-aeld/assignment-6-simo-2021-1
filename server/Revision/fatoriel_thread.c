#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

typedef struct {
    int n;
    long resultat;
} Factorielle;

// Thread qui calcule la factorielle
void *thread_factorielle(void *arg) {
    Factorielle *f = (Factorielle *)arg;
    f->resultat = 1;
    for (int i = 2; i <= f->n; i++) {
        f->resultat *= i;
    }
    printf("Thread : %d! = %ld\n", f->n, f->resultat);
    return NULL;
}

int main() {
    pthread_t thread;

    // ALLOCATION dynamique
    Factorielle *f = malloc(sizeof(Factorielle));
    f->n = 5;

    // Lancer thread
    pthread_create(&thread, NULL, thread_factorielle, f);

    // Attendre la fin
    pthread_join(thread, NULL);

    // Utiliser le résultat
    printf("Main : 5! = %ld\n", f->resultat);

    // LIBERATION (obligatoire après join !)
    free(f);

    return 0;
}