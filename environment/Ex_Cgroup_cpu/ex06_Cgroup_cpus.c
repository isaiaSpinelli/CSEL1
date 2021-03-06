/**
 * Autĥor:	Charbon Yann & Spinelli Isaia
 * Date:	13.11.2020
 * Petite application permettant de valider la capacité des groupes de contrôle de limiter
l’utilisation des CPUs. 
 */
 
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

void child() {
	while(1);   
}

void parent() {
	while(1); 
}

int main ()
{
 
	pid_t pid = fork();
	// child
	if (pid == 0) {
        child();
		
		// parent
	} else if (pid > 0) { 
		
		 parent();
	} else {
		printf("Error fork() \n");
	}
	 
    return 0;
}
