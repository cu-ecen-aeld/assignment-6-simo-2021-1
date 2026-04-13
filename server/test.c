#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

// 👉 FONCTION DU THREAD TEMPS (dédiée au temps)
void* thread_temps(void* arg)
{
    while (1)
    {
        time_t now = time(NULL);
        printf("⏰ Heure actuelle : %s", ctime(&now));
        
        sleep(5); // Attend 1 seconde
    }
    return NULL;
}

int main()
{
    pthread_t thread;

    // 👉 LANCER LE THREAD DANS MAIN
    pthread_create(&thread, NULL, thread_temps, NULL);

    // Main attend le thread (sinon programme se termine tout de suite)
    pthread_join(thread, NULL);

    return 0;
}