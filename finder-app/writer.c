/*# Tester script for assignment 1 and assignment 2
# Author: Arnaud Simo
# 01.12.2025
# Ce programme: fait exactement ce que write.h : il écrivait une chaine de charactère dans un fichier.
# write.c prends un string et l'ecrit dans un fichier.
*/ 
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>  

int main(int argc, char *argv[]) { 
	 
	// Check argument count
    if (argc < 2) {
        printf("Usage: %s \"texte à écrire manque.\"\n", argv[0]);
        return 1;
    }
	   
    const char *filename = argv[1]; //"/tmp/aeld-data/example.txt";
    const char *content = argv[2];  

	 // --- open syslog ---
	openlog("MyLog", LOG_PID, LOG_USER);
	
	syslog(LOG_INFO, "programm starts. ");
	syslog(LOG_DEBUG, "Argument received : %s", content);
    	syslog(LOG_DEBUG, "file names : %s", filename);
	
	
	 // --- Open file ---
	FILE *fp = fopen(filename, "w");
	if (fp == NULL){
		syslog(LOG_ERR, "Error while writing on file: %s", filename);		
		closelog();
		return  1;
	}
	
	syslog(LOG_DEBUG, "File %s opened with success.", filename);
	
	 // --- write on the file ---
    fprintf(fp, "%s\n", content);
    fclose(fp);
	
    syslog(LOG_INFO, "Writting successfully %s", filename);
    syslog(LOG_DEBUG, "Texte written : %s", content);

    // --- Fermeture syslog ---
    closelog();
}
