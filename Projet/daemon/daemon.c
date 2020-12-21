/**
 * Autĥor:	Charbon Yann & Spinelli Isaia
 * Date:	11.12.20
 * File : 	daemon.c
 * Desc. : 	Daemon du projet CSEL1
 * --------------
 * NOTE : Don't forget to compile with -lrt flag
 * --------------
 */
#define _XOPEN_SOURCE 600
#define _DEFAULT_SOURCE

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <grp.h>
#include <pwd.h>
#include <signal.h>
#include <syslog.h>
#include <mqueue.h>
#include <errno.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "ssd1306.h"

#define UNUSED(x) (void)(x)

#define MAX_MQ_SIZE			(100)
#define INCOME_QUEUE_NAME		"/fanpwmsetqueue"
#define OUTCOME_QUEUE_NAME		"/fanpwmgetqueue"

#define MAX_COMMAND_SIZE	(20)

#define FAN_FREQ_FILE		"/sys/class/fan_pwm/device_pwm/parameters/freq"

void set_freq(long freq);
long get_freq();

void lcd_write_header();
void lcd_write_freq(int freq);
void lcd_write_temp(int temp);
void lcd_write_duty(int duty);

static int signal_catched = 0; 

static void catch_signal (int signal)
{
	syslog (LOG_INFO, "signal=%d catched\n", signal);
	signal_catched++;
}

static void fork_process()
{
	pid_t pid = fork();
	switch (pid) {
	case  0: break; 	// child process has been created
	case -1: syslog (LOG_ERR, "ERROR while forking"); exit (1); break;	
	default: exit(0);  	// exit parent process with success
	}
}

int main(int argc, char* argv[])
{
	//UNUSED(argc); UNUSED(argv);

	if(argc == 2){
		if(strcmp(argv[1], "-d") == 0){
			// start as daemon
			printf("Starting as a daemon\n");
			
			// 1. fork off the parent process
			fork_process();

			// 2. create new session
			if (setsid() == -1) {
				syslog (LOG_ERR, "ERROR while creating new session"); 
				exit (1);
			}

			// 3. fork again to get rid of session leading process 
			fork_process();

			// 4. capture all required signals
			struct sigaction act = {.sa_handler = catch_signal,};
			sigaction (SIGHUP,  &act, NULL);  //  1 - hangup
			sigaction (SIGINT,  &act, NULL);  //  2 - terminal interrupt
			sigaction (SIGQUIT, &act, NULL);  //  3 - terminal quit
			sigaction (SIGABRT, &act, NULL);  //  6 - abort
			sigaction (SIGTERM, &act, NULL);  // 15 - termination
			sigaction (SIGTSTP, &act, NULL);  // 19 - terminal stop signal

			// 5. update file mode creation mask
			umask(0027);

			// 6. change working directory to appropriate place
			if (chdir ("/") == -1) {
				syslog (LOG_ERR, "ERROR while changing to working directory"); 
				exit (1);
			}

			// 7. close all open file descriptors
			for (int fd = sysconf(_SC_OPEN_MAX); fd >= 0; fd--) {
				close (fd);
			}

			// 8. redirect stdin, stdout and stderr to /dev/null
			if (open ("/dev/null", O_RDWR) != STDIN_FILENO) {
				syslog (LOG_ERR, "ERROR while opening '/dev/null' for stdin");
				exit (1);
			}
			if (dup2 (STDIN_FILENO, STDOUT_FILENO) != STDOUT_FILENO) {
				syslog (LOG_ERR, "ERROR while opening '/dev/null' for stdout");
				exit (1);
			}
			if (dup2 (STDIN_FILENO, STDERR_FILENO) != STDERR_FILENO) {
				syslog (LOG_ERR, "ERROR while opening '/dev/null' for stderr");
				exit (1);
			}

			// 9. option: open syslog for message logging
			openlog (NULL, LOG_NDELAY | LOG_PID, LOG_DAEMON); 
			syslog (LOG_INFO, "Fan Daemon has started...");
		
			// 10. option: get effective user and group id for appropriate's one
			struct passwd* pwd = getpwnam ("daemon");
			if (pwd == 0) {
				syslog (LOG_ERR, "ERROR while reading daemon password file entry");
				exit (1);
			}

			// 11. option: change root directory
			if (chroot (".") == -1) {
				syslog (LOG_ERR, "ERROR while changing to new root directory");
				exit (1);
			}

			// 12. option: change effective user and group id for appropriate's one
			if (setegid (pwd->pw_gid) == -1) {
				syslog (LOG_ERR, "ERROR while setting new effective group id");
				exit (1);
			}
			if (seteuid (pwd->pw_uid) == -1) {
				syslog (LOG_ERR, "ERROR while setting new effective user id");
				exit (1);
			}
		} else {
			printf("Unkown parameter\n");
			printf("Starting as user app\n");
		}
	} else {
		printf("Starting as user app\n");
	}

	// 13. implement daemon/app body...

	ssd1306_init();    

    lcd_write_header();

	mqd_t income_mq, outcome_mq;
    struct mq_attr income_mq_attr, outcome_mq_attr;
    char income_mq_buffer[MAX_MQ_SIZE + 1];
	char outcome_mq_buffer[MAX_MQ_SIZE + 1];
    int stop_request = 0;

    /* initialize the queue attributes */
    income_mq_attr.mq_flags = 0;
    income_mq_attr.mq_maxmsg = 10;
    income_mq_attr.mq_msgsize = MAX_MQ_SIZE;
    income_mq_attr.mq_curmsgs = 0;

	outcome_mq_attr.mq_flags = 0;
    outcome_mq_attr.mq_maxmsg = 10;
    outcome_mq_attr.mq_msgsize = MAX_MQ_SIZE;
    outcome_mq_attr.mq_curmsgs = 0;

    /* create the message queue */
    income_mq = mq_open(INCOME_QUEUE_NAME, O_CREAT | O_RDONLY, 0644, &income_mq_attr);
    if(income_mq == -1)
		printf("Error : couldn't open income message queue ! errno=%d Exiting \n", errno);

	outcome_mq = mq_open(OUTCOME_QUEUE_NAME, O_CREAT | O_WRONLY, 0644, &outcome_mq_attr);
	if(outcome_mq == -1)
		printf("Error : couldn't open outcome message queue ! errno=%d Exiting \n", errno);

	do {
		
        ssize_t bytes_read;

        /* receive the message */
        bytes_read = mq_receive(income_mq, income_mq_buffer, MAX_MQ_SIZE, NULL);
        if(bytes_read > 0){
			printf("New message\n");
			income_mq_buffer[bytes_read] = '\0';

			char command[MAX_COMMAND_SIZE];
			int value;
			sscanf(income_mq_buffer, "%s %d", command, &value);

			if(!strcmp(command, "stop")){
				stop_request = 1;
			} else if (!strcmp(command, "setfreq")){
				set_freq(value);
				lcd_write_freq(value);
			} else if (!strcmp(command, "getfreq")){
				int freq = get_freq();
				printf("Freq is %d\n", freq);
				sprintf(outcome_mq_buffer, "%d", freq);
				mq_send(outcome_mq, outcome_mq_buffer, MAX_MQ_SIZE, 0);
			}
		}
    } while (!stop_request);

    /* cleanup */
    if(mq_close(income_mq) == -1)
		printf("Error : couldn't close income message queue ! errno=%d Exiting \n", errno);
    if(mq_unlink(INCOME_QUEUE_NAME) == -1)
		printf("Error : couldn't unlink income message queue ! errno=%d Exiting \n", errno);

	if(mq_close(outcome_mq) == -1)
		printf("Error : couldn't close outcome message queue ! errno=%d Exiting \n", errno);
    if(mq_unlink(OUTCOME_QUEUE_NAME) == -1)
		printf("Error : couldn't unlink outcome message queue ! errno=%d Exiting \n", errno);

	syslog (LOG_INFO, "daemon stopped. Number of signals catched=%d\n", signal_catched);
	closelog();

	return 0;
}


void set_freq(long freq)
{
	char freq_str[11];
	sprintf(freq_str,"%ld", freq);

	int f = open (FAN_FREQ_FILE, O_WRONLY);
	if(f == -1)
		printf("Error : couldn't open income FAN_FREQ_FILE ! errno=%d Exiting \n", errno);
	else
		pwrite (f, freq_str, strlen(freq_str), 0);
	close (f);
}

long get_freq()
{
	char freq_str[11] = {'\0'};
	int f = open (FAN_FREQ_FILE, O_RDONLY);
	if(f == -1) {
		printf("Error : couldn't open FAN_FREQ_FILE ! errno=%d Exiting \n", errno);
	} else {
		pread (f, freq_str, 11, 0);
	}
		
	close (f);

	return atoi(freq_str);
}



void lcd_write_header(){
	ssd1306_set_position (0,0);
    ssd1306_puts("CSEL1a - Projet");
    ssd1306_set_position (0,1);
    ssd1306_puts("  Daemon");
    ssd1306_set_position (0,2);
    ssd1306_puts("--------------");	
}

void lcd_write_freq(int freq){
	char freq_str[32];
	sprintf(freq_str, "Freq: %d [Hz]", freq);
    ssd1306_set_position (0,4);
    ssd1306_puts(freq_str);
}

void lcd_write_temp(int temp){
	ssd1306_set_position (0,3);
    ssd1306_puts("Temp: 35'C");
}

void lcd_write_duty(int duty){
	ssd1306_set_position (0,5);
    ssd1306_puts("Duty: 50%");
}

