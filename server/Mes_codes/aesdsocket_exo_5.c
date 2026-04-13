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

// Header for daemon
#include "daemon_utils.h"

#define PORT 9000
#define BUFFER_SIZE 1024
#define FICHIER_SORTIE "/var/tmp/aesdsocketdata"

/*--------------------------------------Global Configs----------------------------------------------*/
// Variable globale "volatile" pour être modifiée par le signal
volatile sig_atomic_t keep_running = 1;
/*--------------------------------------------------------------------------------------------------*/
void handle_signal(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
    printf("\nSignal %d reçu, fermeture en cours...\n", sig);
    keep_running = 0; // On demande à la boucle de s'arrêter
    }
}

// global variable to store the server socket file descriptor, needed for signal handler
int server_fd;
int premiere_connexion = 1;

/*---------------------------------End Global Configs----------------------------------------------*/

int main(int argc, char *argv[]) 
{
    /*--------------------------------------Configs----------------------------------------------*/    
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    struct sockaddr_in address; // Structure pour l'adresse du socket    
    char buffer[BUFFER_SIZE]= {0}; // Buffer pour stocker les données reçues du client
    memset(buffer, 0, BUFFER_SIZE); // Initialise tout à zéro

    // Configurer l'adresse du socket
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT); 
    
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
    if (listen(server_fd, 1) < 0) {
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
    //signal(SIGINT, signal_handler);
    //signal(SIGTERM, signal_handler);    
    int count_connection = 0;
    while (keep_running)
    {                  
       	count_connection++;    	
        printf("\nConnection number: %d\n", count_connection);
        // ACCEPTER UNE ou PLUSIEURS CONNEXIONS                
        int new_socket; //= malloc(sizeof(int)); // Socket pour la connexion client
        new_socket = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);    

        int found_newline = 0; // Flag pour indiquer si une nouvelle ligne a été trouvée
        
        if (new_socket < 0) {
            //free(new_socket); // Toujours libérer si accept échoue
            if (errno == EINTR) break; 
            //perror("Erreur accept");
            //syslog(LOG_ERR, "Accept Connection failed...");
            continue;
        }   

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        printf("Connexion acceptée: %s !\n", client_ip);
        syslog(LOG_INFO, "Accepted connection from %s", client_ip);
        
        // si le serveur reçoit un signal d'arrêt, il ferme la connexion et arrête le serveur
        if (keep_running == 0) {
            close(new_socket);
            printf("Connexion fermée: %s !\n", client_ip);
            //free(new_socket); // Libérer la mémoire allouée pour le socket client en cas d'arrêt du serveur
            continue; // Sortir de la boucle principale pour arrêter le serveur
        }    

        // texte reçu du client, on s'assure que c'est une chaîne de caractères terminée par un null         
        // write data to file
        
        FILE *file = fopen(FICHIER_SORTIE, "a");
        if (file == NULL) {
            perror("Erreur ouverture fichier");
            close(new_socket);
            printf("Connexion fermée: %s !\n", client_ip);
            //free(new_socket); // Libérer la mémoire allouée pour le socket client en cas d'erreur
            continue;
        }
        while(!found_newline){
            //// receive data from client        
            ssize_t val_read = read(new_socket, buffer, BUFFER_SIZE);
            if (val_read < 0) {
                perror("Erreur lecture du client");
                fclose(file); // Fermer le fichier avant de fermer la connexion
                close(new_socket);
                //printf("Connexion fermée...\n");
                //free(new_socket); // Libérer la mémoire allouée pour le socket client en cas d'erreur
                continue;
            }
            //if (val_read == 0) break; // Client a fermé la connexion
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
        //syslog(LOG_INFO, "Received data: %zd", val_read);

        // resend the data to client after writing to file
        // 1. Ouvrir le fichier en lecture
        file = fopen(FICHIER_SORTIE, "r");
        if (file) {
            size_t bytes_read;
            // Lire par blocs pour ne pas saturer la RAM (très important pour AESD)
            while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
                send(new_socket, buffer, bytes_read, 0);
            }
            
            fclose(file);
        }

        // --- ÉTAPE G : Fermeture et Log ---
        close(new_socket);
        //remove("/var/tmp/aesdsocketdata");
        syslog(LOG_INFO, "Closed connection from %s", client_ip);
        
    }// End while (keep_running)
    remove(FICHIER_SORTIE); // Nettoyer le fichier à la fermeture du serveur
    syslog(LOG_INFO, "Caught signal, exiting");
    
    return 0;
}