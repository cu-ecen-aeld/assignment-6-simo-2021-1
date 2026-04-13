#include "threading.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <sys/sysinfo.h>

#include <unistd.h>  // pour sleep ()

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)

//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{
    // Récupérer les paramètres du thread
    struct thread_data* thread_func_args = (struct thread_data *) thread_param;

    // Vérifier si le pointeur est valide
    if (thread_func_args == NULL) {
        perror("thread_func_args est NULL");
        return NULL;
    }

    thread_func_args->thread_complete_success = false;

    // Convertir les millisecondes en secondes pour sleep (ici on simplifie, en réalité il faudrait gérer mieux les ms)
    if (sleep(thread_func_args->wait_to_obtain / 1000) != 0) {
        perror("sleep (wait_to_obtain)");
        return thread_param;
    }

    // Tentative d'obtenir le mutex
    if (pthread_mutex_lock(thread_func_args->mutex) != 0) {
        perror("pthread_mutex_lock");
        return thread_param;
    }
    printf("Le thread a obtenu le mutex.\n");

    // Attendre avant de libérer le mutex
    if (sleep(thread_func_args->wait_to_release / 1000) != 0) {
        perror("sleep (wait_to_release)");
        pthread_mutex_unlock(thread_func_args->mutex);
        return thread_param;
    }

    // Libération du mutex
    if (pthread_mutex_unlock(thread_func_args->mutex) != 0) {
        perror("pthread_mutex_unlock");
        return thread_param;
    }
    printf("Le thread a libéré le mutex.\n");

    thread_func_args->thread_complete_success = true;
    return thread_param;
}


/**
* 1_Start a thread which sleeps @param wait_to_obtain_ms number of milliseconds, then obtains the
* mutex in @param mutex, then holds for @param wait_to_release_ms milliseconds, then releases.
* 
* 2_The start_thread_obtaining_mutex function should only start the thread and should not block
* for the thread to complete.
* 
* 3_The start_thread_obtaining_mutex function should use dynamic memory allocation for thread_data
* structure passed into the thread.  The number of threads active should be limited only by the
* amount of available memory.
* 
* The thread started should return a pointer to the thread_data structure when it exits, which can be used
* to free memory as well as to check thread_complete_success for successful exit.
* If a thread was started succesfully @param thread should be filled with the pthread_create thread ID
* coresponding to the thread which was started.
* @return true if the thread could be started, false if a failure occurred.
*/


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    /**
     * TODO: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     *
     * See implementation details in threading.h file comment block
     */
    sem_t semaphore;
    struct sysinfo info;
    if (sysinfo(&info) != 0) {
        perror("sysinfo");
        return false;
    }

    // Estimation de la mémoire consommée par un thread (à ajuster selon vos besoins)
    size_t memory_per_thread = 1024 * 1024; // 1 Mo par thread
    size_t available_memory = info.freeram;
    int max_active_threads = available_memory / memory_per_thread;

    // Initialiser le sémaphore avec le nombre maximum de threads actifs
    if (sem_init(&semaphore, 0, max_active_threads) != 0) {
        perror("sem_init");
        return false;
    }

    // Vérifier si on peut créer un nouveau thread en utilisant le sémaphore
    if (sem_wait(&semaphore) != 0) {
        perror("sem_wait");
        return false;
    }

    // Allouer de la mémoire pour la structure de données du thread
    struct thread_data *data = (struct thread_data *)malloc(sizeof(struct thread_data));
    if (data == NULL) {
        perror("malloc");
        sem_post(&semaphore); // Relâcher le sémaphore en cas d'erreur
        return false;
    }

    // Initialiser les données du thread
    data->mutex = mutex;
    data->wait_to_obtain = wait_to_obtain_ms;
    data->wait_to_release = wait_to_release_ms;
    data->thread_complete_success = false;

    // Créer le thread
    if (pthread_create(thread, NULL, threadfunc, data) != 0) {
        perror("pthread_create");
        free(data);
        sem_post(&semaphore); // Relâcher le sémaphore en cas d'erreur
        return false;
    }

    return true;
}
