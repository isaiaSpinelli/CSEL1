/**
 * Autĥor:	Isaia Spinelli
 * Date:	16.10.2020
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
	
	FILE * fd;	
	char buff[255];


	
	fd = fopen("/dev/myMisc_0", "w+");
	
	fputs("This is testing for my caractere driver \n", fd);
	
	
	fgets(buff, 255, (FILE*)fd);
	printf("read : %s\n", buff );

	
	
	fclose(fd);

    return 0;
}
