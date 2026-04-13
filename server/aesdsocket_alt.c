/*#  socket based program for assignment 5 part 1
# Author: Arnaud Simo
# 07.01.2026
# Ce programme IMPLEMENTE UN socket server inscrivant des chaines de charactère dans un fichier.
Pour le tester dans un premier temps
	ouvrir une 2e fenetrre: lancer nc localhost 9000
	mais avant il faudrait dans le premiere fenetre lancer: ./aesdsocket

Enfin pour faire fonctionner le programme: 
	dans une fenetre lancer: ./sockettest.sh  (depuis: cd aesd-assignments/assignment-autotest/test/assignment5)
	mais avant il faudrait dans le premiere fenetre lancer: ./aesdsocket
	
Des que tout est bon lance: ./sockettest.sh (de la meme maniere laiser tourner d'abord ./aesdsocket

name: aesdsocket.c

# 
*/

/*--------------------------------------HEADERS----------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>
// to convert IP adress in ASCII
#include <arpa/inet.h>
// Nécessaire pour la gestion des signaux
#include <signal.h> 
//errno.h pour strerror()
#include <errno.h>
// Header for sockets
#include <sys/socket.h>
#include <netinet/in.h>
// Header for daemon
#include "daemon_utils.h"
/*------------------------------------End-HEADERS--------------------------------------------*/

#define PORT 9000       // Port to bind the socket
#define BUFFER_SIZE 1024

/*-------------------------------------- Global ---------------------------------------------*/
// Global Var for Server (ON/OFF) 
volatile sig_atomic_t stop_server = 0;

// Handler for SIGINT (Ctrl+C) & SIGTERM (kill <PID>)
void signal_handler(int sig) 
{
        // Log comme demandé dans l'énoncé
    const char *msg = " Caught signal, exiting \n";
    write(STDERR_FILENO, msg, strlen(msg)); // Remplace printf (async-signal-safe)
    syslog(LOG_INFO, "Caught signal, exiting\n");
    stop_server = 1; // Met la variable à 1 pour arrêter la boucle
}

// Cleanup function for atexit (daemon mode)
void cleanup(const char *filename) 
{
    if (access(filename, F_OK) == 0) {
        if (remove(filename) == 0) {
            syslog(LOG_INFO, "Deleted temporary file %s", filename);
        } else {
            syslog(LOG_ERR, "Failed to delete file %s: %s", filename, strerror(errno));
        }
    }
    closelog();
}
/*---------------------------------------- End Global -------------------------------------------*/


int main(int argc, char *argv[]) 
{

    /*--------------------------------------Configs----------------------------------------------*/
    int is_daemon = 0;
    const char *filename = "/var/tmp/aesdsocketdata";
    int server_fd = -1;       // Initialiser à -1 (sécurité)
    struct sockaddr_in address; // Structure pour l'adresse du socket
    int opt = 1; // Décommenter pour setsockopt
    socklen_t addrlen = sizeof(address);
    
    // Configurer l'adresse du socket
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;         // IPv4
    address.sin_addr.s_addr = INADDR_ANY; // Écouter sur toutes les interfaces (localhost + réseau)
    address.sin_port = htons(PORT);       // Convertir le port en format réseau (big-endian)
    
    // Configuration of SIGINT / SIGTERM
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler; // Associe le signal à la fonction handler
    sigaction(SIGINT, &sa, NULL);   // Gère Ctrl+C
    sigaction(SIGTERM, &sa, NULL);  // Gère kill <PID>
    sigaction(SIGQUIT, &sa, NULL);  // Gère SIGQUIT (ajout pour daemon)
    
    // --- open syslog ---
    openlog("Log_aesdsocket", LOG_PID, LOG_USER);
    /*-------------------------------------End-Configs-------------------------------------------*/
     
    /*--------------------------- option -d (mode daemon) ---------------------------------------*/
    if (argc > 1 && strcmp(argv[1], "-d") == 0) {
        is_daemon = 1;

        // Convertir en daemon (utiliser la fonction de daemon_utils.h)
        if (become_daemon() != 0) {
            syslog(LOG_ERR, "Failed to become daemon: %m");
            closelog();
            return EXIT_FAILURE;
        }

        // Reconfigure syslog for daemon
        closelog();
        openlog("aesdsocket", LOG_PID | LOG_CONS, LOG_USER);
        syslog(LOG_INFO, "aesdsocket : Daemon started on port %d", PORT);

        // Nettoyer le fichier à la fermeture
        //atexit(() -> cleanup(filename)); // Utiliser atexit (pas exit(0) !)
    } else {
        // Mode interactif : afficher des infos dans la console
        printf("aesdsocket : Mode interactif (port %d)\n", PORT);
        printf("Pour lancer en daemon : %s -d\n", argv[0]);
        printf("Arrêter le daemon : kill $(pgrep aesdsocket)\n");
    }
    /*------------------------- End -option -d (mode daemon) ------------------------------------*/         
    
    /*------------------------2.b Opens a stream socket bound to port ---------------------------*/
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
    	syslog(LOG_ERR, "Failed to create server socket: %m");
        closelog(); 
        return EXIT_FAILURE; // Remplacer return -1 par EXIT_FAILURE
    }
    /*-------------------------End -2.b ---------------------------------------------------------*/
    
    /*--------------------------------------Configs Socket--------------------------------------*/
    // Option pour réutiliser le port (évite "port déjà utilisé")
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        syslog(LOG_ERR, "Failed to set socket options: %m");
        close(server_fd);
        closelog(); 
        exit(EXIT_FAILURE);
    }	
   /*-------------------------------- End Configs Socket ---------------------------------------*/

   /*---------------------------------------Bind Socket----------------------------------------*/
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        syslog(LOG_ERR, "Failed to bind socket: %m");
        close(server_fd);
        closelog(); 
        exit(EXIT_FAILURE);
    }
   /*---------------------------------- End Bind Socket ----------------------------------------*/
   
   /*--------------------------2.c listen for and accept connection-----------------------------*/
    if (listen(server_fd, 3) < 0) { // 3 = nombre max de connexions en attente
    	syslog(LOG_ERR, "Failed to listen on socket: %m");
    	close(server_fd);
    	closelog(); 
        exit(EXIT_FAILURE);
    }
    /*--------------------------End 2.c listen for and accept connection------------------------*/
    
    /*----------------------------- Main Loop: Accept connections ------------------------------*/
    int count_connection = 0;
    while (!stop_server) {   	
    	count_connection++;
    	if (!is_daemon) { // Only console output in interactive mode
            printf("\nConnection number: %d\n", count_connection);
            printf("Waiting for connection...\n");
            printf("Server ready : Stream socket bound to port %d\n", PORT);
        }
        
        // Accept new connection
        int new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        //printf("Var: new_socket  %d\n", new_socket);
        // Loop stops when stop_server activated (recieved signal)
        if (stop_server) {
            break;
        }
        
        // Accept (connection) failed
        if (new_socket < 0) {
            syslog(LOG_ERR, "Accept Connection failed : %m");
            continue; // (re)start the loop (wait new client)
        }

	/*---------------------- 2.d: LOG : Accepted connection from XXX -----------------------*/
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &address.sin_addr, client_ip, INET_ADDRSTRLEN);
	syslog(LOG_INFO, "Accepted connection from %s", client_ip);
	/*---------------------------------------- End 2.d: ------------------------------------*/
        
        /*---------------------------------- 2.e: Data Reception -------------------------------*/
	char recv_buffer[BUFFER_SIZE] = {0};  // Buffer fixe for datas
	char *packet_buffer = NULL;  // dynamique buffer to save
	size_t packet_len = 0;	
	ssize_t bytes_read = 0;
	
	// Read client data until newline or connection closed
	while ((bytes_read = read(new_socket, recv_buffer, BUFFER_SIZE - 1)) > 0 && !stop_server) 
	{		    
	    recv_buffer[bytes_read] = '\0'; // Terminer la chaîne
	    char *newline_ptr = strchr(recv_buffer, '\n');
	    
	    if (newline_ptr != NULL) {
		// === PAQUET COMPLET (détecté \n) ===
		size_t part_len = newline_ptr - recv_buffer + 1; // +1 pour inclure le \n
		
	        char *temp_buffer = realloc(packet_buffer, packet_len + part_len);
	        if (temp_buffer == NULL) {
                    syslog(LOG_ERR, "Realloc failed: %s", strerror(errno));
                    free(packet_buffer);
                    close(new_socket);
                    break;
	        }
	        packet_buffer = temp_buffer;
	        memcpy(packet_buffer + packet_len, recv_buffer, part_len);
	        packet_len += part_len;

		// Écrire le paquet complet dans le fichier
		FILE *fp = fopen(filename, "a");
		if (fp == NULL) { // Supprimer la ligne dupliquée
                    syslog(LOG_ERR, "Failed to open file %s for writing: %s", filename, strerror(errno));
                    free(packet_buffer);
                    close(new_socket);
                    break;
                }
		
	        size_t written = fwrite(packet_buffer, 1, packet_len, fp);
	        if (written != packet_len) {
	            syslog(LOG_ERR, "Failed to write to file %s: %s", filename, strerror(errno));
	        }
	      	fflush(fp);
                fclose(fp);
                syslog(LOG_DEBUG, "Wrote %zu bytes to %s", written, filename);

		// Envoyer le fichier complet au client
		fp = fopen(filename, "r");
		if (fp == NULL) {
     		    syslog(LOG_ERR, "Failed to open file %s for reading: %s", filename, strerror(errno));
                    free(packet_buffer);
                    close(new_socket);
                    break;
		}
		
		char file_buffer[BUFFER_SIZE];
		ssize_t file_bytes;
		while ((file_bytes = fread(file_buffer, 1, BUFFER_SIZE, fp)) > 0 && !stop_server) {
	            ssize_t sent = send(new_socket, file_buffer, file_bytes, 0);
                    if (sent == -1) {
                        syslog(LOG_ERR, "Failed to send data to client %s: %m", client_ip);
                        break;
                    } else if (sent < file_bytes) {
                        syslog(LOG_WARNING, "Partial send to client %s: sent %zd of %zd bytes",
                               client_ip, sent, file_bytes);
                    }
		}
		fclose(fp);
		syslog(LOG_DEBUG, "File content sent to client");

		// Réinitialiser le buffer de paquet
		free(packet_buffer);
		packet_buffer = NULL;
		packet_len = 0;

		// Gérer le reste des données (après le \n)
		size_t remaining_len = bytes_read - part_len;
		if (remaining_len > 0) {
		    packet_buffer = malloc(remaining_len);
		    if (packet_buffer == NULL) {
	                syslog(LOG_ERR, "Malloc failed: %s", strerror(errno));
	                close(new_socket);
	                break;
	            }	
		    memcpy(packet_buffer, newline_ptr + 1, remaining_len);
		    packet_len = remaining_len;
		}
	     } else {
		// === PAQUET INCOMPLET (pas de \n) ===
	        char *temp_buffer = realloc(packet_buffer, packet_len + bytes_read);
	        if (temp_buffer == NULL) {
		    syslog(LOG_ERR, "Realloc failed: %s", strerror(errno));
                    free(packet_buffer);
                    close(new_socket);
                    break;
	        }
	        packet_buffer = temp_buffer;
	        memcpy(packet_buffer + packet_len, recv_buffer, bytes_read);
	        packet_len += bytes_read;
	    }
	    // Reset receive buffer
            memset(recv_buffer, 0, sizeof(recv_buffer));
	}

	// Gestion erreur read()
	if (bytes_read < 0 && !stop_server) {
	    syslog(LOG_ERR, "Error reading from client %s: %m", client_ip);
	}
	
	// Nettoyage du buffer de paquet
	if (packet_buffer != NULL) {
	    free(packet_buffer);
	}

        // Log fermeture connexion
        syslog(LOG_INFO, "Closed connection from %s", client_ip);    
        close(new_socket);
    }
    
    /*------------------------ Server Shutdown Cleanup -------------------------*/
    // Close server socket
    if (server_fd != -1) {
        close(server_fd);
    }
    
    // Supprimer le fichier temporaire
	if (access(filename, F_OK) == 0) {
	if (remove(filename) == 0) {
	    syslog(LOG_INFO, "Deleted temporary file %s", filename);
	} else {
	    syslog(LOG_ERR, "Failed to delete file %s: %s", filename, strerror(errno));
	}
    }
    closelog();
    
    // Console output only in interactive mode
    if (!is_daemon) {
        printf("\nProgram stopped properly!\n");
    }

    return 0;
}
