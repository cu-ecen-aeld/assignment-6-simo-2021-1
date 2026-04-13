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

#define PORT 9000
#define BUFFER_SIZE 1024
#define FICHIER_SORTIE "/var/tmp/aesdsocketdata"

pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;
int server_fd;   

void signal_handler(int sig) {
    printf("\nSignal reçu (%d), fermeture en cours...\n", sig);
    close(server_fd);
    remove(FICHIER_SORTIE); // Supprimer le fichier comme demandé
    exit(0);
}

// handler for client connection
void* handle_client(void* arg) {
    int new_socket = *(int*)arg;
    free(arg); // Libérer la mémoire allouée pour le socket         

    char buffer[BUFFER_SIZE] = {0};
    // Lire les données envoyées par le client
    ssize_t valread = read(new_socket, buffer, BUFFER_SIZE);
    if (valread < 0) {
        perror("Erreur lecture");
        close(new_socket);
        return NULL;
    }
    if (valread >= BUFFER_SIZE) {
        buffer[BUFFER_SIZE - 1] = '\0';
    } else {
        buffer[valread] = '\0';
    }
    printf("Données reçues : %s\n", buffer);        
    pthread_mutex_lock(&file_mutex);

    // Ouvrir le fichier en mode ajout et verrouiller tout le traitement pour éviter les conflits d'accès concurrent
    FILE *fp = fopen(FICHIER_SORTIE, "a");
    if (!fp) {
        perror("Erreur ouverture fichier");
        pthread_mutex_unlock(&file_mutex);
        close(new_socket);
        return NULL;
    }
    // Écrire les données reçues dans le fichier
    if (fwrite(buffer, 1, valread, fp) != (size_t)valread) {
        perror("Erreur écriture fichier");
        fclose(fp);
        pthread_mutex_unlock(&file_mutex);
        close(new_socket);
        return NULL;
    }
    fclose(fp);

    // Ouvrir le fichier en mode lecture pour renvoyer les données au client
    fp = fopen(FICHIER_SORTIE, "r");
    if (!fp) {
        perror("Erreur ouverture fichier");
        pthread_mutex_unlock(&file_mutex);
        close(new_socket);
        return NULL;
    }

    char file_buffer[BUFFER_SIZE];
    size_t bytes_read;
    while ((bytes_read = fread(file_buffer, 1, sizeof(file_buffer), fp)) > 0) {
        size_t total_sent = 0;
        while (total_sent < bytes_read) {
            ssize_t sent = send(new_socket, file_buffer + total_sent, bytes_read - total_sent, 0);
            if (sent < 0) {
                perror("Erreur envoi");
                fclose(fp);
                pthread_mutex_unlock(&file_mutex);
                close(new_socket);
                return NULL;
            }
            total_sent += (size_t)sent;
        }
    }
    if (ferror(fp)) {
        perror("Erreur lecture fichier");
        fclose(fp);
        pthread_mutex_unlock(&file_mutex);
        close(new_socket);
        return NULL;
    }

    fclose(fp);
    pthread_mutex_unlock(&file_mutex);
    printf("Données écrites dans %s\n", FICHIER_SORTIE);
    close(new_socket);
    //free(new_socket);
    return NULL;
}

void init_file() {
    // supprime tout le contenu du fichier avant d'écrire les nouvelles données
    FILE *fp = fopen(FICHIER_SORTIE, "w");
    if (!fp) {
        perror("Erreur ouverture fichier");
        pthread_mutex_unlock(&file_mutex);
        //close(new_socket);
        return;
    }
    fclose(fp);    
}

void* tache_timestamp(void* arg) {
    while(1) { // Boucle infinie
        // 1. Attendre 10 secondes
        sleep(10); 

        // 2. Action à faire (écrire dans le fichier)
        printf("DEBUG: J'écris le timestamp dans le fichier...\n");

        // 3. Obtenir le timestamp actuel
        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        char timestamp[64];
        strftime(timestamp, sizeof(timestamp), "timestamp: %a %b %d %Y:%H:%M:%S %z\n", tm_info);

        // 4. Écrire le timestamp dans le fichier
        pthread_mutex_lock(&file_mutex);
        FILE *fp = fopen(FICHIER_SORTIE, "a");
        if (!fp) {
            perror("Erreur ouverture fichier");
            pthread_mutex_unlock(&file_mutex);
            return NULL;
        }
        fprintf(fp, "%s", timestamp);
        fclose(fp);
        pthread_mutex_unlock(&file_mutex);

        // 5. Afficher un message de debug
        printf("DEBUG: Timestamp écrit dans le fichier: %s", timestamp);               

        // Ici, vous ajouterez votre code fopen/strftime/fprintf/fclose
        // N'oubliez pas le MUTEX si le serveur écrit aussi dans ce fichier !
    }
    return NULL;
}

int main() {     
    struct sockaddr_in server_addr;
    init_file(); // Initialiser le fichier en supprimant son contenu avant de démarrer le serveur

    pthread_t tid_temps;

    // On lance le thread de temps au démarrage du serveur
    //pthread_create(&tid_temps, NULL, tache_timestamp, NULL);
       
    pthread_detach(tid_temps);

        // Configuration of SIGINT / SIGTERM
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler; // Associe le signal à la fonction handler
    sigaction(SIGINT, &sa, NULL);   // Gère Ctrl+C
    sigaction(SIGTERM, &sa, NULL);  // Gère kill <PID>
    sigaction(SIGQUIT, &sa, NULL);  // Gère SIGQUIT (ajout pour daemon)

    // --------------------------
    // CREATE SOCKET
    // --------------------------
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Erreur socket");
        exit(EXIT_FAILURE);
    }

    int opt = 1; // activer l'option SO_REUSEADDR pour permettre de réutiliser le port immédiatement après la fermeture du serveur
    int restart_server = setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)); 
        if (restart_server < 0) {    
        perror("Erreur setsockopt");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Config adresse + port
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // --------------------------
    // BIND (attacher au port)
    // --------------------------
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Erreur bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // --------------------------
    // ECOUTER
    // --------------------------
    if (listen(server_fd, 1) < 0) {
        perror("Erreur listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Serveur en attente sur le port %d...\n", PORT);

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);    

    while (1)
    {
        // --------------------------
        // ACCEPTER UNE ou PLUSIEURS CONNEXIONS
        // --------------------------
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int *new_socket = malloc(sizeof(int)); // Allouer de la mémoire pour le socket client
        if (!new_socket) {
            perror("Erreur malloc");
            continue;
        }

        *new_socket = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        printf("\nConnection number: %p\n", (void*)new_socket);
        if (*new_socket < 0) {
            perror("Erreur accept");
            free(new_socket); // Libérer la mémoire allouée pour le socket client en cas d'erreur
            continue;
        }

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        printf("Connexion acceptée: %s !\n", client_ip);

        pthread_t tid_client;

        // Créer un thread pour gérer la connexion client
        int pt  = pthread_create(&tid_client, NULL, handle_client, new_socket);
        if (pt != 0) {
            perror("pthread_create");
            close(*new_socket);
            printf("Connexion fermée: %s !\n", client_ip);
            free(new_socket); // Libérer la mémoire allouée pour le socket client en cas d'erreur
            continue;
        }

        pthread_detach(tid_client);
        // pthread_join(tid_client, NULL); // Attendre que le thread se termine avant d'accepter une nouvelle connexion
        
    }
    
    close(server_fd);
    printf("Serveur arrêté.\n");

    return 0;
}