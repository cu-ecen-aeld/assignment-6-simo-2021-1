#include <stdio.h>
#include <pthread.h>

// Structure pour passer des données au thread (comme un "objet mathématique")
typedef struct {
    int debut;
    int fin;
    int resultat;
} CalculSomme;

// Fonction du thread : calcule la somme
void *thread_somme(void *arg) {
    CalculSomme *s = (CalculSomme *)arg;
    s->resultat = 0;

    for (int i = s->debut; i <= s->fin; i++) {
        s->resultat += i;
    }

    printf("Thread : Somme = %d\n", s->resultat);
    return NULL;
}

int main() {
    pthread_t thread;
    CalculSomme s = {1, 10, 0};

    // Lancer le thread
    pthread_create(&thread, NULL, thread_somme, &s);

    // ATTENDRE que le thread finisse (JOIN)
    printf("Main attend le thread...\n");
    pthread_join(thread, NULL);

    // Résultat disponible
    printf("Main : Somme finale = %d\n", s.resultat);

    return 0;
}