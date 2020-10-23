/**
 * Autĥor:	Isaia Spinelli
 * Date:	23.10.2020
 * Utilisation de Select via le driver /Pilote_ex2/ex7_Op_Bloq
 */

// getpagesize
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
// mmap
#include <sys/mman.h>
// open
#include <fcntl.h>


/**
 * Main program computing the Fibonacci numbers for a given sequence starting 
 * from 0 to a number specified at the command line. 
 */
int main ()
{
	
	int fd;
	
	// pour le select
	fd_set rfds;
	int retval;
	
	int count=0;
	
	
	// Ouvre le fichier /dev/mem
	if ( (fd = open("/dev/button_nano",O_RDWR|O_SYNC)) < 0 ) {
		fprintf(stderr,"ouverture du fichier /dev/mydevice” impossible\n(err=%d / file=%s / line=%d)\n", fd, __FILE__,__LINE__);
		return EXIT_FAILURE;
	}
	
	
	// Fait le lien avec le device /dev/uio1 pour le select
	FD_ZERO(&rfds);
	FD_SET(fd, &rfds);
	
	while (1){
		// Attend jusqu'a que le device envoie une interruption
		retval = select(fd+1, &rfds, NULL, NULL, NULL);	
		// S'il y a une erreur
		if (retval == -1)
		   perror("select()");
		// Si le device est pret
		else if (retval > 0) {
			count++;
			fprintf(stdout, "count = %d (r=%d)\n", count, retval );	

		}
	}
	
	
	close(fd);
	
    return 0;
}
