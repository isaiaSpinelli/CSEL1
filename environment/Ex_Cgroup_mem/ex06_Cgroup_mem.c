/**
 * Autĥor:	Charbon Yann & Spinelli Isaia
 * Date:	20.11.2020
 * Petite application permettant de valider la capacité des groupes de contrôle de limiter
l’utilisation de la mémoire. 
 */
 
// getpagesize
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include <string.h>

#define NB_BLOCK 		50
#define SIZE_BLOCK_MB 	1
#define NB_BYTE			1024

#define BYTE_ALLOC 		SIZE_BLOCK_MB * NB_BYTE * NB_BYTE


int main ()
{
	void * ptr_tab[NB_BLOCK];
	int i;
	
	for (i=0; i<NB_BLOCK; ++i){
		
		printf("malloc n = %d\n", i);
		sleep(1);
		
		ptr_tab[i] = malloc(BYTE_ALLOC);
		if (ptr_tab[i] == NULL){
			printf("malloc failed ! \n");
		} else {
			memset(ptr_tab[i], 0, BYTE_ALLOC);
		}
		
	} 
	
	for (i=0; i<NB_BLOCK; ++i){
		free(ptr_tab[i]); 
	} 
	 	
    return 0;
}
