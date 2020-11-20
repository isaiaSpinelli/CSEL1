/**
 * Autĥor:	Isaia Spinelli
 * Date:	13.11.2020
 * Utilisation de socketpair 
 */
 
#define _GNU_SOURCE       

// getpagesize
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include <signal.h>
#include <sys/socket.h>
#include <string.h>

#include <sched.h>

void catch_signal(int signo)
{
	printf("signal= %d catched and ignored\n", signo);
	if (signo == 2)
		exit(0);
}   


void child(int socket) {

    int i;
    const int NBSEND = 5 ;
        
    for (i = 0; i < NBSEND ; ++i) {
		// sleep 1s
		sleep(1);
		if (i == NBSEND-1) {
			const char hello[] = "exit";
			write(socket, hello, sizeof(hello));
		} else {
			// send message
			const char hello[] = "I am a child";
			write(socket, hello, sizeof(hello));
		}
	}

	close(socket);
	
	while(1); // for test use cpu 0    
}

void parent(int socket) {
    int end = 0;
    char * exitIs = NULL;
    char buf[1024];
    
    while (!end) {
		int n = read(socket, buf, sizeof(buf));
		printf("Parent get : '%.*s'\n", n, buf);
		exitIs = strstr(buf, "exit");
		if (exitIs != NULL) {
			end = 1;
		}
	}
	
	close(socket);

	while(1); // for test use cpu 1
}

/**
 * Main program computing the Fibonacci numbers for a given sequence starting 
 * from 0 to a number specified at the command line. 
 */
int main ()
{
	int fd[2];
	static const int parentsocket = 0;
    static const int childsocket = 1;
    
	int i = 0;
	printf("Hello %d\n",SIGRTMAX);
	
	// catch all signal
	struct sigaction act = {.sa_handler = catch_signal,};
	for (i=0; i < SIGRTMAX ; i++){
		sigaction (i,  &act, NULL); 
	}
	
	// AF_UNIX - PF_LOCAL
	// SOCK_STREAM SOCK_DGRAM
	 if (socketpair(AF_UNIX, SOCK_STREAM, 0, fd) < 0) {
	  perror("opening stream socket pair");
	  exit(1);
	}

	pid_t pid = fork();
	// child
	if (pid == 0) {
		// use core 0
		cpu_set_t set;
		CPU_ZERO(&set);
		CPU_SET(0, &set);
		int ret = sched_setaffinity(0, sizeof(cpu_set_t), &set);
		if (ret == -1){
			perror("sched_setaffinity");
			exit(1);
		}
			

		printf("I am child\n");
		close(fd[parentsocket]); /* Close the parent file descriptor */
        child(fd[childsocket]);
        printf("child bye !\n");
		
		// parent
	} else if (pid > 0) { 
		
		// use core 1
		cpu_set_t set;
		CPU_ZERO(&set);
		CPU_SET(1, &set);
		int ret = sched_setaffinity(0, sizeof(set), &set);
		if (ret == -1){
			perror("sched_setaffinity");
			exit(1);
		}
		
		printf("I am parent\n");
		close(fd[childsocket]); /* Close the child file descriptor */
        parent(fd[parentsocket]);
        printf("Parent bye !\n");
        
		
	} else {
		printf("Error fork() \n");
	}
	 
	 	
    return 0;
}
