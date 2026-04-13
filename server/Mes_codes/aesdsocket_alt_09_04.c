/**********************************************************************
 * Programme socket serveur SIMPLE
 * - Accepte UNE connexion
 * - Reçoit une chaîne de caractères via: echo "Bonjour3" | nc localhost 9000 
name: simple_server.c
 * - L'écrit DANS UN FICHIER
 * - Ferme la connexion
 * 
 * Ce programme est un exemple de serveur TCP simple qui écoute sur le port 9000.
 * Il accepte une connexion à la fois, reçoit des données du client, les écrit dans un fichier,
 * puis ferme la connexion. Le serveur continue à écouter pour de nouvelles connexions après chaque client
 * Note: Ce code est conçu pour être simple et didactique, et n'est pas optimisé pour la performance ou la sécurité.
 * cmd pour tester: 
 *  echo "Bonjour3" | nc localhost 9000
 *      ./server/sockettest.sh
 *      ./full-test.sh
 *      ./server/sockettest.sh
 *      ./full-test.sh  
 *********************************************************************/

//#define _POSIX_C_SOURCE 200809L // Pour strnlen et d'autres fonctions POSIX
#define _XOPEN_SOURCE 700  // Indispensable AVANT les includes

#define _DEFAULT_SOURCE  // Permet d'activer la fonction daemon()

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <syslog.h>
#include <signal.h>
#include <errno.h>   // Pour errno et EINTR

#include <sys/queue.h> // Pour les listes chaînées (TAILQ) si besoin

#include <signal.h>
#include <string.h> // Pour memset


// Header for daemon
#include "daemon_utils.h"

#define PORT 9000
#define BUFFER_SIZE 1024
#define FICHIER_SORTIE "/var/tmp/aesdsocketdata"

/*--------------------------------------Global Configs----------------------------------------------*/

typedef struct thread_node {
    pthread_t thread;
    struct thread_node *next;
} thread_node_t;

thread_node_t *thread_list_head = NULL;

// Variable globale "volatile" pour être modifiée par le signal
volatile int  keep_running = 1;
pthread_mutex_t list_mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex pour synchroniser l'accès à la liste des threads

// global variable to store the server socket file descriptor, needed for signal handler
int server_fd;
int premiere_connexion = 1;
/*--------------------------------------------------------------------------------------------------*/

/*--------------------------------------Global Fonctions--------------------------------------------*/

void handle_signal(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
    //printf("\nSignal %d reçu, fermeture en cours...\n", sig);
    write(STDOUT_FILENO, "\nSignal reçu, arrêt...\n", 23);
    syslog(LOG_INFO, "Caught signal %d, exiting", sig);
    keep_running = 0; // On demande à la boucle de s'arrêter
    shutdown(server_fd, SHUT_RDWR); // Réveille accept() immédiatement
    }
}

// 👉 FONCTION DU THREAD TEMPS (dédiée au temps)
void* thread_temps(void* arg)
{
    while (keep_running)
    {
        time_t now = time(NULL);
        printf("⏰ Heure actuelle : %s", ctime(&now));    

        //protect the file writing with a mutex to avoid concurrent writes from multiple threads
        pthread_mutex_lock(&list_mutex);
        FILE *file = fopen(FICHIER_SORTIE, "a");
            if (file == NULL) {
                perror("Erreur ouverture fichier");            
            }else {                
                // strftime pour formater la date et l'heure de manière plus lisible
                char time_str[100];
                strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&now));
                //fprintf(file, "Heure actuelle : %s", ctime(&now));                
                fclose(file);
                pthread_mutex_unlock(&list_mutex);
            }    
            sleep(20); // Attend 20 secondes
        }
    return NULL;
}

// arret voire google AI...
void* gerer_client(void *arg) {
    int socket_client = *(int*) arg;
    free(arg);  // libère la mémoire allouée pour le socket

    int found_newline = 0; // Flag pour indiquer si une nouvelle ligne a été trouvée
    char buffer[BUFFER_SIZE] = {0}; // Buffer pour stocker les données reçues du client
    memset(buffer, 0, BUFFER_SIZE); // Initialise tout à zéro
    
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);        
    char client_ip[INET_ADDRSTRLEN];   
    
    if (getpeername(socket_client, (struct sockaddr *)&client_addr, &client_len) == 0) {
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);    
        printf("Connexion acceptée: %s !\n", client_ip);
        syslog(LOG_INFO, "Accepted connection from %s", client_ip);
    } else {
        perror("Erreur getpeername");
        close(socket_client);
        return NULL;
    }     
    
    pthread_mutex_lock(&list_mutex); // Verrou pour éviter les conflits d'écriture concurrente
    // --- ÉTAPE F : Lire/Écrire avec le client ---
    FILE *file = fopen(FICHIER_SORTIE, "a");
        if (file == NULL) {
            perror("Erreur ouverture fichier");
            close(socket_client);
            //printf("Connexion fermée: %s !\n", client_ip);            
            //continue;
            return NULL;
        }
        while(!found_newline){
            //// receive data from client        
            ssize_t val_read = read(socket_client, buffer, BUFFER_SIZE);
            if (val_read < 0) {
                perror("Erreur lecture du client");
                fclose(file); // Fermer le fichier avant de fermer la connexion
                close(socket_client);
                //printf("Connexion fermée...\n");
                //free(new_socket); // Libérer la mémoire allouée pour le socket client en cas d'erreur
                break;
            }
            if (val_read == 0) break; // Client a fermé la connexion            
            fwrite(buffer, 1, val_read, file);
            
            for (int i = 0; i < val_read; i++) {
                if (buffer[i] == '\n') {
                    found_newline = 1; // Nouvelle ligne trouvée, on peut arrêter de lire
                    break;
                }                              
            }                  
        }
        fclose(file); // Indispensable pour valider l'écriture sur le disque          
        //printf("Données reçues du client: %s\n", buffer);
        syslog(LOG_INFO, "Received data: %s", buffer);
        
        // resend the data to client after writing to file
        // 1. Ouvrir le fichier en lecture
        file = fopen(FICHIER_SORTIE, "r");
        if (file) {
            size_t bytes_read;
            // Lire par blocs pour ne pas saturer la RAM (très important pour AESD)
            while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
                
                send(socket_client, buffer, bytes_read, 0);
                
            }            
            fclose(file);
        }
    pthread_mutex_unlock(&list_mutex); // Déverrou après envoi au client
        // --- ÉTAPE G : Fermeture et Log ---        
        //remove("/var/tmp/aesdsocketdata");    
    syslog(LOG_INFO, "Closed connection from %s", client_ip);
    close(socket_client);
    return NULL;
} //ARRET gerer_client()

/*---------------------------------Global Fonctions----------------------------------------------*/


int main(int argc, char *argv[]) 
{
    /*--------------------------------------Configs----------------------------------------------*/        
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    struct sockaddr_in address; // Structure pour l'adresse du socket
    
    // Configurer l'adresse du socket
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Configurer les handlers de signal pour SIGINT et SIGTERM
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));  
    sa.sa_handler = handle_signal; // Associe le signal à la fonction handler
    sa.sa_flags = 0; // TRÈS IMPORTANT : permet d'interrompre accept()
    sigaction(SIGINT, &sa, NULL);   // Gère Ctrl+C
    sigaction(SIGTERM, &sa, NULL);  // Gère kill <PID>
    sigaction(SIGQUIT, &sa, NULL);  // Gère SIGQUIT (ajout pour daemon)    
    
    // --- open syslog ---
    openlog("Log_aesdsocket", LOG_PID, LOG_USER);   
    /*-------------------------------------End-Configs-------------------------------------------*/         
    
    // open a socket bound on port 9000
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Erreur socket");
        exit(EXIT_FAILURE);
    }

    int opt = 1; // activer l'option SO_REUSEADDR pour permettre de réutiliser le port immédiatement après la fermeture du serveur
    int restart_server = setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)); 
        if (restart_server < 0) {    
        perror("Erreur setsockopt");
        syslog(LOG_INFO, "Erreur setsockopt sur le port %d", PORT);
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    
    // BIND (attacher au port)    
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Erreur bind");
        close(server_fd);
        exit(EXIT_FAILURE);    }

    // accepter une  connexions
    if (listen(server_fd, 5) < 0) {
        perror("Erreur listen");
        syslog(LOG_INFO, "Erreur listen sur le port %d", PORT);
        close(server_fd);
        exit(EXIT_FAILURE);    }

    // Vérifier si l'utilisateur a passé l'argument -d
    if (argc > 1 && strcmp(argv[1], "-d") == 0) {
        // daemon(1, 0) : 
        // 1 = reste dans le répertoire actuel (ne va pas à la racine /)
        // 0 = redirige les sorties standard (printf) vers /dev/null
        if (daemon(1, 0) == -1) {
            perror("Erreur daemon");
            //syslog(LOG_ERR, "Failed to become daemon...");
            exit(EXIT_FAILURE);
        }
    }
    printf("Serveur en attente sur le port %d...\n", PORT);
    syslog(LOG_INFO, "Server started on port %d", PORT);    
    int count_connection = -1; // Compteur de connexions pour afficher le numéro de connexion

    pthread_t thread_temps_id, th; // Thread pour le temps et thread pour gérer les clients
    pthread_create(&thread_temps_id, NULL, thread_temps, NULL);
    while (keep_running)
    {                  
       	count_connection++;    	
        if (count_connection == 0) {
            printf("\nPremière connexion entrante...\n");
        } else {
        printf("\nConnection number: %d\n", count_connection);
        }
        
        // ACCEPTER UNE ou PLUSIEURS CONNEXIONS                
        int *new_socket = malloc(sizeof(int)); // Socket pour la connexion client        
        *new_socket = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);    

        if (*new_socket == -1) {
            free(new_socket); // Toujours libérer si accept échoue
            if (errno == EINTR) break; // Si accept est interrompu par un signal, sortir de la boucle pour arrêter le serveur
            perror("Erreur accept");
            syslog(LOG_ERR, "Accept Connection failed...");
            continue;
        }
               
        if (*new_socket < 0) {
            //free(new_socket); // Toujours libérer si accept échoue
            if (errno == EINTR) break; 
            //perror("Erreur accept");
            //syslog(LOG_ERR, "Accept Connection failed...");
            continue;
        }           
        
        
        pthread_create(&th, NULL, gerer_client, new_socket);
        
        
        // Ajouter le thread à la liste chaînée
        thread_node_t *node = malloc(sizeof(thread_node_t));
        if (node) {
            node->thread = th;
            node->next = NULL;
            pthread_mutex_lock(&list_mutex);
            node->next = thread_list_head;
            thread_list_head = node;
            pthread_mutex_unlock(&list_mutex);
        } else {
            perror("malloc node");
        }

        // si le serveur reçoit un signal d'arrêt, il ferme la connexion et arrête le serveur
        if (keep_running == 0) {
            close(*new_socket);
            printf("Connexion fermée: %s !\n", inet_ntoa(client_addr.sin_addr)  );
            //free(new_socket); // Libérer la mémoire allouée pour le socket client en cas d'arrêt du serveur
            continue; // Sortir de la boucle principale pour arrêter le serveur
        }            
    }// End while (keep_running)   
    
   // À la place de pthread_join(th, NULL);
    pthread_mutex_lock(&list_mutex);
    thread_node_t *current = thread_list_head;
    while (current != NULL) {
        pthread_join(current->thread, NULL);        
        thread_node_t *temp = current;
        current = current->next;
        free(temp); // Libère le nœud de la liste
    }

    pthread_mutex_unlock(&list_mutex);
    pthread_join(thread_temps_id, NULL);
    pthread_mutex_destroy(&list_mutex);

    syslog(LOG_INFO, "Tentative de suppression du fichier...");
    remove(FICHIER_SORTIE); // Nettoyer le fichier à la fermeture du serveur
    syslog(LOG_INFO, "Caught signal, exiting");    
    close(server_fd);
    return 0;
}