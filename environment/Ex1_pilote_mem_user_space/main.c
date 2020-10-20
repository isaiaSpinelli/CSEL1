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

typedef volatile unsigned int 		vuint;

#define BASE_ADDR_CHIP_ID      			0x01c14200
#define OFFSET_CHIP_ID					0x200
#define OFFSET_CHIP_ID_INT				(0x200 / 4)

/**
 * Main program computing the Fibonacci numbers for a given sequence starting 
 * from 0 to a number specified at the command line. 
 */
int main ()
{
	u_int32_t chipID[4];
	
	int fd;
	int i;
	
	// Pointeur sur la zone memoire 
	vuint *seg = NULL;
	
	
	// Ouvre le fichier /dev/mem
	if ( (fd = open("/dev/mem",O_RDWR|O_SYNC)) < 0 ) {
		fprintf(stderr,"ouverture du fichier /dev/mem impossible\n(err=%d / file=%s / line=%d)\n", fd, __FILE__,__LINE__);
		return EXIT_FAILURE;
	}
	
	// getpagesize() = 4096
	// map la zone memoire voulu
	off_t size = getpagesize();
	off_t addr = BASE_ADDR_CHIP_ID;
	off_t ofs = addr % size;
	off_t offset = addr - ofs;
	
	fprintf(stdout, "size = %lx - addr = %lx - ofs = %lx - offset = %lx\n", size, addr, ofs, offset);	
	
	if ( (seg = (vuint *) mmap(0, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, offset )) == NULL){
		fprintf(stderr,"mmap impossible\n(err=%d / file=%s / line=%d)\n", fd, __FILE__,__LINE__);
		return EXIT_FAILURE;
	}
	
	// Lecture et affichage CHIP ID
	for (i=0; i < 4; ++i){
		chipID[i] = seg[OFFSET_CHIP_ID_INT+i];
	}
	fprintf(stdout, "CHIP ID = %u - %u - %u - %u\n", chipID[0],chipID[1],chipID[2],chipID[3]);	
	
	munmap(0, size);
	close(fd);

    return 0;
}
