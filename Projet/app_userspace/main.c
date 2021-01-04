/**
 * Autĥor:	Charbon Yann & Spinelli Isaia
 * Date:	11.12.20
 * File : 	main.c
 * Desc. : 	Application userspace  du projet CSEL1
  * --------------
 * NOTE : Don't forget to compile with -lrt flag for LD
 * --------------
 */
 
#define _GNU_SOURCE       

// getpagesize
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <sched.h>
#include <errno.h>

#include <mqueue.h>

#define MAX_MQ_SIZE			(100)
#define SEND_QUEUE_NAME		"/fanpwmsetqueue"
#define RECV_QUEUE_NAME		"/fanpwmgetqueue"

#define MAX_COMMAND_SIZE	(20)

int main(int argc, char* argv[]){
	
	mqd_t send_mq, recv_mq;
    char send_buffer[MAX_MQ_SIZE];
	char recv_buffer[MAX_MQ_SIZE];

    /* open the mail queue */
    send_mq = mq_open(SEND_QUEUE_NAME, O_WRONLY);
    if(send_mq == -1)
		printf("Error : couldn't open send message queue ! errno=%d Exiting \n", errno);

	recv_mq = mq_open(RECV_QUEUE_NAME, O_RDONLY);
    if(recv_mq == -1)
		printf("Error : couldn't open recv message queue ! errno=%d Exiting \n", errno);

	if(argc > 1){
		if(!strcmp(argv[1], "--set-freq")){
			int freq;
			sscanf(argv[2], "%d", &freq);
			sprintf(send_buffer, "setfreq %d", freq);
			mq_send(send_mq, send_buffer, MAX_MQ_SIZE, 0);

			ssize_t bytes_read;

			/* receive the message */
			bytes_read = mq_receive(recv_mq, recv_buffer, MAX_MQ_SIZE, NULL);
			if(bytes_read > 0){
				recv_buffer[bytes_read] = '\0';
				if(!strcmp(recv_buffer, "AUTO")){
					printf("Fan is in auto mode. Cannot set freq.\n");
				}				
			}

		} else if (!strcmp(argv[1], "--get-freq")){
			sprintf(send_buffer, "getfreq %d", 0);
			mq_send(send_mq, send_buffer, MAX_MQ_SIZE, 0);

			ssize_t bytes_read;

			/* receive the message */
			bytes_read = mq_receive(recv_mq, recv_buffer, MAX_MQ_SIZE, NULL);
			if(bytes_read > 0){
				recv_buffer[bytes_read] = '\0';
				int value;
				sscanf(recv_buffer, "%d", &value);
				printf("freq set to %d\n", value);
			}

		} else if (!strcmp(argv[1], "--stopdaem")){
			sprintf(send_buffer, "stop %d", 0);
			mq_send(send_mq, send_buffer, MAX_MQ_SIZE, 0);
		} else if (!strcmp(argv[1], "--get-mode")){
			sprintf(send_buffer, "getmode %d", 0);
			mq_send(send_mq, send_buffer, MAX_MQ_SIZE, 0);

			ssize_t bytes_read;

			/* receive the message */
			bytes_read = mq_receive(recv_mq, recv_buffer, MAX_MQ_SIZE, NULL);
			if(bytes_read > 0){
				recv_buffer[bytes_read] = '\0';
				int value;
				sscanf(recv_buffer, "%d", &value);
				printf("mode is set to %d\n", value);
			}

		} else if (!strcmp(argv[1], "--set-mode")){
			int mode;
			sscanf(argv[2], "%d", &mode);
			if(mode >= 0 && mode <= 1){				
				sprintf(send_buffer, "setmode %d", mode);
				mq_send(send_mq, send_buffer, MAX_MQ_SIZE, 0);
			} else {
				printf("Mode can be 0: auto, 1:manual\n");
			}
			
		} else if (!strcmp(argv[1], "--get-temp")){
			sprintf(send_buffer, "gettemp %d", 0);
			mq_send(send_mq, send_buffer, MAX_MQ_SIZE, 0);

			ssize_t bytes_read;

			/* receive the message */
			bytes_read = mq_receive(recv_mq, recv_buffer, MAX_MQ_SIZE, NULL);
			if(bytes_read > 0){
				recv_buffer[bytes_read] = '\0';
				int value;
				sscanf(recv_buffer, "%d", &value);
				printf("CPU temp is at %d\n", value);
			}

		} else if(!strcmp(argv[1], "--help")) {
			printf("Please specify option : \n");
			printf("--set-freq [value], --get-freq\n");			
			printf("--set-mode [0,1], --get-mode\n");
			printf("--get-temp\n");
			printf("--stopdaemon\n");
		} else {
			printf("Unkown option\n");
		}
	} else {
		printf("Please specify option : \n");
		printf("--set-freq [value], --get-freq\n");		
		printf("--set-mode [0,1], --get-mode\n");
		printf("--get-temp\n");
		printf("--stopdaemon\n");
	}



    /* cleanup */
    if(mq_close(send_mq) == -1)
		printf("Error : couldn't close send message queue ! errno=%d Exiting \n", errno);

	if(mq_close(recv_mq) == -1)
		printf("Error : couldn't close recv message queue ! errno=%d Exiting \n", errno);


	return 0;
}
