#include <stdio.h>
#include <pthread.h>

// Données pour thread 1 : Somme
typedef struct {
    int debut, fin, res;
} Somme;

// Données pour thread 2 : Factorielle
typedef struct {
    int n;
    long res;
} Facto;

// Thread 1 : Somme
void *thread_somme(void *arg) {
    Somme *s = arg;
    s->res = 0;
    for (int i=s->debut; i<=s->fin; i++) s->res += i;
    printf("✅ Somme 1→100 = %d\n", s->res);
    return NULL;
}

// Thread 2 : Factorielle
void *thread_facto(void *arg) {
    Facto *f = arg;
    f->res = 1;
    for (int i=2; i<=f->n; i++) f->res *= i;
    printf("✅ Factorielle 10! = %ld\n", f->res);
    return NULL;
}

int main() {
    pthread_t t1, t2;

    Somme s = {1, 100, 0};
    Facto f = {10, 0};

    // 🚀 LANCE LES DEUX THREADS EN MÊME TEMPS
    pthread_create(&t1, NULL, thread_somme, &s);
    pthread_create(&t2, NULL, thread_facto, &f);

    // ⏳ Attendre la fin des deux
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    printf("\nMain : Les deux calculs sont terminés !\n");
    return 0;
}