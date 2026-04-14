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
 *  echo "Lundi" | nc localhost 9000 & echo "Mardi" | nc localhost 9000   
 *      ./server/sockettest.sh
 *      ./full-test.sh
 *      ./server/sockettest.sh
 *      ./full-test.sh  
 * 
 * modified by: Tchuinkou Arnaud 
 * Date: 2026-04-08 
 * Assignment 5 Part 1 Instructions
 * Link: https://www.coursera.org/learn/linux-system-programming-introduction-to-buildroot/supplement/c7ZrG/assignment-5-part-1-instructions
 *********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

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

// Analyse des options : -d
int opt;
int daemon_mode = 0;
/*--------------------------------------------------------------------------------------------------*/

// global variable to store the server socket file descriptor, needed for signal handler
int server_fd = -1; // Socket pour le serveur, accessible globalement pour pouvoir le fermer dans le handler de signal
int premiere_connexion = 1;

/*---------------------------------End Global Configs----------------------------------------------*/

/*--------------------------------------Global Fonctions--------------------------------------------*/

void handle_signal(int sig) {   
    keep_running = 0; // On demande à la boucle de s'arrêter
    const char *msg = "\nSignal reçu !\n";
    (void)write(STDOUT_FILENO, msg, sizeof(msg)-1); // Affiche un message simple pour indiquer que le signal a été reçu (sans utiliser printf qui n'est pas sûr dans les handlers de signal)      
    if (server_fd != -1) {
        //shutdown(server_fd, SHUT_RDWR); // Réveille accept() immédiatement
        close(server_fd); // Ferme le socket du serveur pour libérer la ressource et faire échouer les futurs accept()
    }    
}

/*---------------------------------End Global Functions----------------------------------------------*/

int main( int argc, char *argv[])
{
    /*--------------------------------------Configs----------------------------------------------*/            
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    struct sockaddr_in address; // Structure pour l'adresse du socket    
    char buffer[BUFFER_SIZE]= {0}; // Buffer pour stocker les données reçues du client
    //memset(buffer, 0, BUFFER_SIZE); // Initialise tout à zéro

    // Configurer l'adresse du socket
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT); 
    
    // Configurer les handlers de signal pour SIGINT et SIGTERM        
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));  
    sa.sa_handler = handle_signal; // Associe le signal à la fonction handler
    // Permet à accept() d'être interrompu par un signal
    sa.sa_flags = 0; 

    // On bloque d'autres signaux pendant le handler
    sigemptyset(&sa.sa_mask);
    sigaddset(&sa.sa_mask, SIGINT);
    sigaddset(&sa.sa_mask, SIGTERM);
    sigaddset(&sa.sa_mask, SIGQUIT);

    sigaction(SIGINT,  &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGQUIT, &sa, NULL);

    // --- open syslog ---
    openlog("Log_aesdsocket", LOG_PID, LOG_USER);   
    /*-------------------------------------End-Configs-------------------------------------------*/   
    /*-------------------------------------daemon_mode-------------------------------------------*/   
    // Analyse des options : -d
    while ((opt = getopt(argc, argv, "d")) != -1) {
        switch (opt) {
            case 'd':
                daemon_mode = 1;
                break;
            default:
                fprintf(stderr, "Usage: %s [-d]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    // Si mode démon demandé, se détacher
    if (daemon_mode) {
        // Méthode manuelle ou appel à daemon()
        if (daemon(1, 0) == -1) {  // 1 = chdir("/"), 0 = redirige stdin/out/err vers /dev/null
            perror("daemon");
            exit(EXIT_FAILURE);
        }
    }
    /*-------------------------------------End-daemon_mode-------------------------------------------*/   

    // open a socket bound on port 9000
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Erreur socket");
        exit(EXIT_FAILURE);
    }
    int value = 1;// Permet de réutiliser le port immédiatement après la fermeture du serveur
    int restart_server = setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value)); 
        if (restart_server < 0) {    
        perror("Erreur setsockopt");
        syslog(LOG_INFO, "Erreur setsockopt sur le port %d", PORT);
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    
    // BIND (attacher au port)    
    int bind_result = bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    if (bind_result < 0) {
        perror("Erreur bind");
        close(server_fd);
        exit(EXIT_FAILURE);    }

    // accepter une  connexions
    int listen_result = listen(server_fd, 5); // 5 = nombre de connexions en attente autorisées
    if (listen_result < 0) {
        perror("Erreur listen");
        syslog(LOG_INFO, "Erreur listen sur le port %d", PORT);
        close(server_fd);
        exit(EXIT_FAILURE);    }    

    printf("Serveur en attente sur le port %d...\n", PORT);
    syslog(LOG_INFO, "Server started on port %d", PORT);    
    //int count_connection = -1; // Compteur de connexions pour afficher le numéro de connexion

    while (keep_running) // Boucle principale du serveur, continue tant que keep_running est vrai (non interrompu par un signal)
    {                  
        // ACCEPTER UNE ou PLUSIEURS CONNEXIONS                
        int new_socket; //= malloc(sizeof(int)); // Socket pour la connexion client
        new_socket = accept(server_fd, (struct sockaddr *)&client_addr, &client_len); 

        if (new_socket == -1) {
            // EINTR signifie que accept a été interrompu par Ctrl+C
            if (errno == EINTR || !keep_running) {
                keep_running = 0; // Assurer que la boucle s'arrête
                break; // ON SORT DU WHILE ICI
            }
            perror("accept");
            break; // Sortir de la boucle en cas d'erreur d'accept
        }

        // AJOUTER CES LIGNES
        struct timeval timeout = {
            .tv_sec = 2,   // 2 secondes max
            .tv_usec = 0
        };
        setsockopt(new_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
        setsockopt(new_socket, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        printf("Connexion acceptée: %s !\n", client_ip);
        syslog(LOG_INFO, "Accepted connection from %s", client_ip);
        
        // si le serveur reçoit un signal d'arrêt, il ferme la connexion et arrête le serveur
        if (keep_running == 0) {
            close(new_socket);
            printf("Connexion fermée: %s !\n", client_ip);
            syslog(LOG_INFO, "Closed connection from %s", client_ip);            
            continue; // Sortir de la boucle principale pour arrêter le serveur
        }    

        // texte reçu du client, on s'assure que c'est une chaîne de caractères terminée par un null         
        // write data to file
        
        FILE *file = fopen(FICHIER_SORTIE, "a");
        if (file == NULL) {
            perror("Erreur ouverture fichier");
            close(new_socket);
            printf("Connexion fermée: %s !\n", client_ip);            
            continue;
        }
        size_t current_size = 0;
        char *full_packet = NULL;
        char recv_buf[512]; 
        

        // BOUCLE CORRECTE
        while (keep_running) {  
            ssize_t nr = recv(new_socket, recv_buf, sizeof(recv_buf), 0);

            if (nr <= 0) {
                break;
            }

            char *tmp = realloc(full_packet, current_size + nr);
            if (!tmp) {
                free(full_packet);
                fclose(file);
                close(new_socket);
                continue;
            }
            full_packet = tmp;
            memcpy(full_packet + current_size, recv_buf, nr);
            current_size += nr;

            if (memchr(full_packet, '\n', current_size)) {
                break;
            }
        }

        //printf("Received data from client: %.*s\n", (int)current_size, full_packet); // Afficher le paquet complet reçu du client
        // 4. On écrit le paquet complet d'un coup dans le fichier
        size_t written = fwrite(full_packet, 1, current_size, file);            
        
        if (written != current_size) {
            // Erreur ! On logue et on ferme
            syslog(LOG_ERR, "Erreur d'écriture : attendu %zu, écrit %zu", current_size, written);
            perror("fwrite");
        } else {
            // Succès ! On force l'écriture physique sur le disque
            fflush(file);             
        }
        
        // 5. ON LIBÈRE (très important pour Valgrind)
        free(full_packet);
        fclose(file); // Fermer le fichier pour libérer les ressources et éviter les problèmes de verrouillage
        //free(buffer);

        // resend the data to client after writing to file
        // 1. Ouvrir le fichier en lecture
        file = fopen(FICHIER_SORTIE, "r");
        if (file == NULL) {
            perror("Erreur ouverture fichier pour lecture");
            close(new_socket);
            printf("Connexion fermée: %s !\n", client_ip);            
            continue;
        }
        if (file) {
            size_t bytes_read;
            // Lire par blocs pour ne pas saturer la RAM (très important pour AESD)            
            while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
                //int result = send(new_socket, buffer, bytes_read, 0);
                if (send(new_socket, buffer, bytes_read, 0) == -1) {
                    perror("send");
                    break;
                }
                //printf("Sent back to client: %.*s\n", (int)bytes_read, buffer); // Afficher ce qui est renvoyé au client
            }            
            //fclose(file); 
            //close(new_socket);            
        } 
        fclose(file); // Fermer le fichier après la lecture pour libérer les ressources et éviter les problèmes de verrouillage

    // --- ÉTAPE G : Fermeture et Log ---
    //free(buffer);
    close(new_socket);
    
    printf("Connexion fermée: %s !\n", client_ip);
    syslog(LOG_INFO, "Closed connection from %s", client_ip);
            
    }// End while (keep_running)

    // 1. Log officiel exigé par l'énoncé
    syslog(LOG_INFO, "Caught signal, exiting");

    // 2. Nettoyage des threads (comme vu avec la liste chaînée)
    // ... boucle pthread_join ...

    // 3. Suppression du fichier (remove ou unlink)
    unlink(FICHIER_SORTIE); 

    // 4. Fermeture finale
    close(server_fd);
    closelog();    

    return 0;
}