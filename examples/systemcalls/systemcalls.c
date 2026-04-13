#include "systemcalls.h"

#include <stdio.h>    // for perror()
#include <stdlib.h>   // for abort()
#include <unistd.h>   // for fork(), execv(), dup2(), close()
#include <sys/wait.h> // for waitpid(), WIFEXITED(), WEXITSTATUS()
#include <fcntl.h>    // for open()

/**
 * @param cmd the command to execute with system()
 * @return true if the command in @param cmd was executed
 *   successfully using the system() call, false if an error occurred,
 *   either in invocation of the system() call, or if a non-zero return
 *   value was returned by the command issued in @param cmd.
 *
 * TODO
 *  Call the system() function with the command set in the cmd
 *   and return a boolean true if the system() call completed with success
 *   or false() if it returned a failure
 *
 *
*/
bool do_system(const char *cmd)
{
    // Test 1 : Commande SUCCESS
    int ret = system(cmd);
    
   if (ret != -1 && WEXITSTATUS(ret) == 0) {
        //printf("Commande réussie !\n");
        printf("Cmd succeded: %s\n", cmd);
        printf("                           \n");
        return true;
    } else {
        printf("Cmd failed !\n");
        return false;
    }
}

/**
* @param count -The numbers of variables passed to the function. The variables are command to execute.
*   followed by arguments to pass to the command
*   Since exec() does not perform path expansion, the command to execute needs
*   to be an absolute path.
* @param ... - A list of 1 or more arguments after the @param count argument.
*   The first is always the full path to the command to execute with execv()
*   The remaining arguments are a list of arguments to pass to the command in execv()
* @return true if the command @param ... with arguments @param arguments were executed successfully
*   using the execv() call, false if an error occurred, either in invocation of the
*   fork, waitpid, or execv() command, or if a non-zero return value was returned
*   by the command issued in @param arguments with the specified arguments.
*/

bool do_exec(int count, ...)
{
    printf("--START__do_exec()__ --- \n\n");
    // Initialisation de la liste d'arguments variables
    va_list args;
    va_start(args, count);

    // Étape 1 : Construction du tableau de commandes pour execv()
    char *command[count + 1];
    int i;
    for (i = 0; i < count; i++)
    {
        command[i] = va_arg(args, char *); // Remplissage du tableau avec les arguments
    }
    command[count] = NULL; // Fin du tableau par NULL (obligatoire pour execv())

    // Ligne inutile (peut être supprimée)
    /* command[count] = command[count];  */

    // Étape 2 : Création du processus fils avec fork()
    pid_t pid = fork();
    if (pid == -1)
    {
        // Erreur : échec de la création du fils
        va_end(args);
        perror("fork() a échoué"); // Correction : error() → perror() (standard)
        return false;
    }

    // Étape 3 : Code exécuté par le fils
    if (pid == 0)
    {
        va_end(args); // Libération de la liste d'arguments (fils)
        
       // AJOUT DU PRINTF POUR AFFICHER LA COMMANDE
        printf("###Commande exécutée par execv() : ");
        for (i = 0; i < count; i++) // On parcourt tous les arguments
        {
            printf("%s ", command[i]); // On affiche chaque argument (cmd + args)
        }
        printf("\n"); // Saut de ligne pour la lisibilité
        
        execv(command[0], command); // Exécution de la commande	
        // Si execv() retourne, c'est qu'il y a eu une erreur
        perror("execv() a échoué");
        exit(EXIT_FAILURE);
    }
    
    // Étape 4 : Code exécuté par le père
    else
    {
        int status;

        // Attente de la fin du fils
        if (waitpid(pid, &status, 0) == -1)
        {
            va_end(args);
            perror("waitpid() a échoué");
            return false;
        }

        /*
         * TODO:
         *   Execute a system command by calling fork, execv(),
         *   and wait instead of system (see LSP page 161).
         *   Use the command[0] as the full path to the command to execute
         *   (first argument to execv), and use the remaining arguments
         *   as second argument to the execv() command.
         */

        // Libération de la liste d'arguments (père)
        va_end(args);

        // Vérification du statut de sortie du fils
        if (WIFEXITED(status))
        {
            // Le fils a terminé normalement : vérifier le code de retour
            if (WEXITSTATUS(status) == 0)
            {
            	printf("Le fils a terminé normalement\n");
            	printf("\n");
                return true; // Commande réussie
            }
            else
            {
                printf("Le fils n'a pas terminé normalement\n");
                printf("\n");
                return false; // Commande échouée
            }
        }
        else
        {
            // Le fils a été interrompu (ex: signal) → échec
            return false;
        }
    }

    // Ce return n'est jamais atteint (sécurité)
    // return true;
}

/**
* @param outputfile - The full path to the file to write with command output.
*   This file will be closed at completion of the function call.
* All other parameters, see do_exec above
*/
bool do_exec_redirect(const char *outputfile, int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    // command[count] = command[count];


/*
 * TODO
 *   Call execv, but first using https://stackoverflow.com/a/13784315/1446624 as a refernce,
 *   redirect standard out to a file specified by outputfile.
 *   The rest of the behaviour is same as do_exec()
 *
*/
    // Step: 2 create first (chlid) fork()
    int kidpid;
    int fd = open(outputfile, O_WRONLY|O_TRUNC|O_CREAT, 0644);
    
    if (fd < 0) { perror ("open"); va_end(args); abort(); }
	    switch (kidpid = fork()) {
	    	case -1: perror ("fork"); va_end(args); abort();
	    	case  0: 
	    		if   (dup2(fd, 1) < 0) { perror("dup2"); abort(); }
	    		close(fd);
	    		execv(command[0],command); perror("execv"); abort();
	    	default:
	    		close(fd);		
		} 
     // code of father
	int status;
	// wait code of father ends
	if (waitpid(kidpid, &status, 0) == -1) 
	{
		va_end(args);
		perror("waitpid() a échoué");
		return false;				
	}
    //clean list of args
    va_end(args);
    if (WIFEXITED(status)) 
     {
     	if (WEXITSTATUS(status) == 0 ) 
     	{
     		return true;
     	}
  	else
	{
		return false;
	}	
     }
   else
     {
     	return false;
     }	     

    //va_end(args);

    //return true;
}
