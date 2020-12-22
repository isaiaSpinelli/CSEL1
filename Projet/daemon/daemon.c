/**
 * Autĥor:	Charbon Yann & Spinelli Isaia
 * Date:	11.12.20
 * File : 	daemon.c
 * Desc. : 	Daemon du projet CSEL1
 * --------------
 * NOTE : Don't forget to compile with -pthread and -lrt flag for LD
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

#include <pthread.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <sys/epoll.h>
#include <sys/timerfd.h>

#include "ssd1306.h"

#define UNUSED(x) (void)(x)

/*
 * status led - gpioa.10 --> gpio10
 * power led  - gpiol.10 --> gpio362
 */
#define GPIO_EXPORT		"/sys/class/gpio/export"
#define GPIO_UNEXPORT	"/sys/class/gpio/unexport"
#define GPIO_PWR_LED	"/sys/class/gpio/gpio362"
#define GPIO_K1		    "/sys/class/gpio/gpio0"
#define GPIO_K2		    "/sys/class/gpio/gpio2"
#define GPIO_K3		    "/sys/class/gpio/gpio3"
#define LED				"362"
#define K1              "0"
#define K2              "2"
#define K3              "3"

#define MAX_MQ_SIZE			(100)
#define INCOME_QUEUE_NAME		"/fanpwmsetqueue"
#define OUTCOME_QUEUE_NAME		"/fanpwmgetqueue"

#define MAX_COMMAND_SIZE	(20)

#define FAN_FREQ_FILE		"/sys/class/fan_pwm/device_pwm/parameters/freq"
#define FAN_MODE_FILE		"/sys/class/fan_pwm/device_pwm/parameters/mode"
#define FAN_CPUTEMP_FILE	"/sys/class/fan_pwm/device_pwm/parameters/temp_cpu"


void set_freq(int freq);
long get_freq();
void set_mode(int mode);
int get_mode();
int get_cpu_temp();

void lcd_write_header();
void lcd_write_freq(int freq);
void lcd_write_temp(int temp);
void lcd_write_mode(int mode);

int open_pwr_led();
int open_k1();
int open_k1();
int open_k3();

struct itimerspec ts;

struct file_desc {
	int cpu_temp_fd;
	int freq_fd;
	int mode_fd;
	int timer_fd;
	int led_pwr_fd;
	int k1_fd;
	int k2_fd;
	int k3_fd;	
};

void* events_thread();

pthread_mutex_t lcd_mutex_lock;

void freq_handler(struct file_desc file_descriptors);
void cpu_temp_handler(struct file_desc file_descriptors);
void timer_handler(struct file_desc file_descriptors);
void k1_handler(struct file_desc file_descriptors);
void k2_handler(struct file_desc file_descriptors);
void k3_handler(struct file_desc file_descriptors);

void set_timer(struct file_desc file_descriptors);
void reset_timer(struct file_desc file_descriptors);

static int signal_catched = 0; 

static int stop_request = 0;

struct file_desc file_descriptors;

int cur_freq = 0;
int cur_mode = 0;
int cur_cpu_temp = 0;

void increment_freq();
void decrement_freq();

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
			printf("Starting as user app. To start as daemon, start with -d flag\n");
		}
	} else {
		printf("Starting as user app. To start as daemon, start with -d flag\n");
	}	

	// 13. implement daemon/app body...
	ssd1306_init();    

    lcd_write_header();

	cur_mode = get_mode();
	lcd_write_mode(cur_mode);


	pthread_t events_pthread;
	int ret = pthread_create(&events_pthread, NULL, &events_thread, NULL);
	if (ret == -1)
		printf("Error : couldn't create pthread ! errno=%d Exiting \n", errno);

	
	mqd_t income_mq, outcome_mq;
    struct mq_attr income_mq_attr, outcome_mq_attr;
    char income_mq_buffer[MAX_MQ_SIZE + 1];
	char outcome_mq_buffer[MAX_MQ_SIZE + 1];   

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
			printf("New message in message queue\n");
			income_mq_buffer[bytes_read] = '\0';

			char command[MAX_COMMAND_SIZE];
			int value;
			sscanf(income_mq_buffer, "%s %d", command, &value);

			if(!strcmp(command, "stop")){
				stop_request = 1;
			} else if (!strcmp(command, "setfreq")){
				if(cur_mode == 1){
					set_freq(value);
					sprintf(outcome_mq_buffer, "OK");
					mq_send(outcome_mq, outcome_mq_buffer, MAX_MQ_SIZE, 0);
				} else {
					sprintf(outcome_mq_buffer, "AUTO");
					mq_send(outcome_mq, outcome_mq_buffer, MAX_MQ_SIZE, 0);
				}			

			} else if (!strcmp(command, "getfreq")){
				int freq = get_freq();
				sprintf(outcome_mq_buffer, "%d", freq);
				mq_send(outcome_mq, outcome_mq_buffer, MAX_MQ_SIZE, 0);

			} else if (!strcmp(command, "setmode")){
				cur_mode = value;
				set_mode(cur_mode);
				pthread_mutex_lock(&lcd_mutex_lock);
				lcd_write_mode(cur_mode);
				pthread_mutex_unlock(&lcd_mutex_lock);

			} else if (!strcmp(command, "getmode")){
				int mode = get_mode();
				sprintf(outcome_mq_buffer, "%d", mode);
				mq_send(outcome_mq, outcome_mq_buffer, MAX_MQ_SIZE, 0);
			} else if (!strcmp(command, "gettemp")){
				int temp = get_cpu_temp();
				sprintf(outcome_mq_buffer, "%d", temp);
				mq_send(outcome_mq, outcome_mq_buffer, MAX_MQ_SIZE, 0);
			}
		}
    } while (!stop_request);

    /* cleanup */
    if(mq_close(income_mq) == -1){
		syslog(LOG_ERR, "Error : couldn't close income message queue ! errno=%d Exiting \n", errno);
		printf("Error : couldn't close income message queue ! errno=%d Exiting \n", errno);
	}		
    if(mq_unlink(INCOME_QUEUE_NAME) == -1){
		syslog(LOG_ERR, "Error : couldn't unlink income message queue ! errno=%d Exiting \n", errno);
		printf("Error : couldn't unlink income message queue ! errno=%d Exiting \n", errno);
	}
		

	if(mq_close(outcome_mq) == -1){
		syslog(LOG_ERR, "Error : couldn't close outcome message queue ! errno=%d Exiting \n", errno);
		printf("Error : couldn't close outcome message queue ! errno=%d Exiting \n", errno);
	}
		
    if(mq_unlink(OUTCOME_QUEUE_NAME) == -1){
		syslog (LOG_ERR, "Error : couldn't unlink outcome message queue ! errno=%d Exiting \n", errno);
		printf("Error : couldn't unlink outcome message queue ! errno=%d Exiting \n", errno);
	}		

	syslog (LOG_INFO, "daemon stopped. Number of signals catched=%d\n", signal_catched);
	closelog();

	return 0;
}


void set_freq(int freq)
{
	char freq_str[11];
	sprintf(freq_str,"%d", freq);
	pwrite (file_descriptors.freq_fd, freq_str, strlen(freq_str), 0);
}

long get_freq()
{
	return cur_freq;
}

void set_mode(int mode)
{
	char mode_str[11];
	sprintf(mode_str,"%d", mode);

	int f = open (FAN_MODE_FILE, O_WRONLY);
	if(f == -1)
		printf("Error : couldn't open income FAN_MODE_FILE ! errno=%d Exiting \n", errno);
	else
		pwrite (f, mode_str, strlen(mode_str), 0);
	close (f);
}


int get_mode()
{
	char mode_str[5] = {'\0'};
	int f = open (FAN_MODE_FILE, O_RDONLY);
	if(f == -1) {
		printf("Error : couldn't open FAN_MODE_FILE ! errno=%d Exiting \n", errno);
	} else {
		pread (f, mode_str, sizeof(mode_str), 0);
	}
		
	close (f);

	return atoi(mode_str);
}

int get_cpu_temp()
{
	return cur_cpu_temp;
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
	char freq_str[15];
	sprintf(freq_str, "Freq: %d [Hz]", freq);
    ssd1306_set_position (0,4);
    ssd1306_puts(freq_str);
}

void lcd_write_temp(int temp){
	char cpu_temp_str[15];
	sprintf(cpu_temp_str, "CPU temp: %d", temp);
	ssd1306_set_position (0,5);
    ssd1306_puts(cpu_temp_str);
}

void lcd_write_mode(int mode){
	ssd1306_set_position (0,3);
    if(mode == 0){
		ssd1306_puts("Auto mode   ");
	} else {
		ssd1306_puts("Manual mode ");
	}
    
}

int open_pwr_led()
{
	// unexport pin out of sysfs (reinitialization)
	int f = open (GPIO_UNEXPORT, O_WRONLY);
	write (f, LED, strlen(LED));
	close (f);

	// export pin to sysfs
	f = open (GPIO_EXPORT, O_WRONLY);
	write (f, LED, strlen(LED));
	close (f);	

	// config pin
	f = open (GPIO_PWR_LED "/direction", O_WRONLY);
	write (f, "out", 3);
	close (f);

	// open gpio value attribute
 	f = open (GPIO_PWR_LED "/value", O_RDWR);
	return f;
}


int open_k1()
{
	// unexport pin out of sysfs (reinitialization)
	int f = open (GPIO_UNEXPORT, O_WRONLY);
	write (f, K1, strlen(K1));
	close (f);

	// export pin to sysfs
	f = open (GPIO_EXPORT, O_WRONLY);
    write (f, K1, strlen(K1));
	close (f);	

	// config pin
    f = open (GPIO_K1 "/direction", O_WRONLY);
	write (f, "in", 2);
	close (f);
	f = open (GPIO_K1 "/edge", O_WRONLY);
	write (f, "both", 4);
	close (f);

	// open gpio value attribute
 	f = open (GPIO_K1 "/value", O_RDONLY);
	return f;
}

int open_k2()
{
	// unexport pin out of sysfs (reinitialization)
	int f = open (GPIO_UNEXPORT, O_WRONLY);
	write (f, K2, strlen(K2));
	close (f);

	// export pin to sysfs
	f = open (GPIO_EXPORT, O_WRONLY);
    write (f, K2, strlen(K2));
	close (f);	

	// config pin
    f = open (GPIO_K2 "/direction", O_WRONLY);
	write (f, "in", 2);
	close (f);
	f = open (GPIO_K2 "/edge", O_WRONLY);
	write (f, "both", 4);
	close (f);

	// open gpio value attribute
 	f = open (GPIO_K2 "/value", O_RDONLY);
	return f;
}

int open_k3()
{
	// unexport pin out of sysfs (reinitialization)
	int f = open (GPIO_UNEXPORT, O_WRONLY);
	write (f, K3, strlen(K3));
	close (f);

	// export pin to sysfs
	f = open (GPIO_EXPORT, O_WRONLY);
    write (f, K3, strlen(K3));
	close (f);	

	// config pin
    f = open (GPIO_K3 "/direction", O_WRONLY);
	write (f, "in", 2);
	close (f);
	f = open (GPIO_K3 "/edge", O_WRONLY);
	write (f, "both", 4);
	close (f);

	// open gpio value attribute
 	f = open (GPIO_K3 "/value", O_RDONLY);
	return f;
}



void* events_thread(){	

	int led = open_pwr_led();
	int k1 = open_k1();
	int k2 = open_k2();
	int k3 = open_k3();

	int cpu_temp_fd = open (FAN_CPUTEMP_FILE, O_RDONLY);
	if(cpu_temp_fd == -1)
		printf("Error : couldn't open income FAN_CPUTEMP_FILE ! errno=%d Exiting \n", errno);

	int freq_fd = open (FAN_FREQ_FILE, O_RDWR);
	if(freq_fd == -1) {
		printf("Error : couldn't open FAN_FREQ_FILE ! errno=%d Exiting \n", errno);
	}

	// Create new timerfd for blink period
	int timerfd = timerfd_create(CLOCK_MONOTONIC,0);
	if (timerfd == -1) {
		printf("timerfd_create failure: errno=%d\n", errno);
	}

	ts.it_value.tv_nsec = 0;
	ts.it_value.tv_sec = 1;
	ts.it_interval.tv_nsec = 0;
	ts.it_interval.tv_sec = 0;


	// Create new epoll context
    int epfd = epoll_create1(0);
    if(epfd == -1){     /* error*/
        printf("Error : couldn't create epoll context ! errno=%d Exiting \n", errno);
    }

	// Create epoll_events
	struct epoll_event freq_fd_event = {
		.events = EPOLLIN,
		.data.fd = freq_fd,
		.data.ptr = freq_handler
	};

	struct epoll_event cpu_temp_fd_event = {
		.events = EPOLLIN,
		.data.fd = cpu_temp_fd,
		.data.ptr = cpu_temp_handler
	};

	struct epoll_event timerfd_event = {
		.events = EPOLLIN,
		.data.fd = timerfd,
		.data.ptr = timer_handler
	};

	struct epoll_event k1_event = {
		.events = EPOLLPRI,
		.data.fd = k1,
		.data.ptr = k1_handler
	};

	struct epoll_event k2_event = {
		.events = EPOLLPRI,
		.data.fd = k2,
		.data.ptr = k2_handler
	};

	struct epoll_event k3_event = {
		.events = EPOLLPRI,
		.data.fd = k3,
		.data.ptr = k3_handler
	};


	// Register events
	int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, freq_fd, &freq_fd_event);
	if (ret == -1){ /* error*/
		printf("Error : couldn't add event to epoll context ! errno=%d Exiting \n", errno);
	}

	ret = epoll_ctl(epfd, EPOLL_CTL_ADD, cpu_temp_fd, &cpu_temp_fd_event);
	if (ret == -1){ /* error*/
		printf("Error : couldn't add event to epoll context ! errno=%d Exiting \n", errno);
	}

	ret = epoll_ctl(epfd, EPOLL_CTL_ADD, timerfd, &timerfd_event);
	if (ret == -1){ /* error*/
		printf("Error : couldn't add event to epoll context ! errno=%d Exiting \n", errno);
	}

	ret = epoll_ctl(epfd, EPOLL_CTL_ADD, k1, &k1_event);
	if (ret == -1){ /* error*/
		printf("Error : couldn't add event to epoll context ! errno=%d Exiting \n", errno);
	}

	ret = epoll_ctl(epfd, EPOLL_CTL_ADD, k2, &k2_event);
	if (ret == -1){ /* error*/
		printf("Error : couldn't add event to epoll context ! errno=%d Exiting \n", errno);
	}

	ret = epoll_ctl(epfd, EPOLL_CTL_ADD, k3, &k3_event);
	if (ret == -1){ /* error*/
		printf("Error : couldn't add event to epoll context ! errno=%d Exiting \n", errno);
	}


	struct epoll_event events[7];
	int nr;
	
	file_descriptors.freq_fd = freq_fd;
	file_descriptors.cpu_temp_fd = cpu_temp_fd;
	file_descriptors.timer_fd = timerfd;
	file_descriptors.led_pwr_fd = led;
	file_descriptors.k1_fd = k1;
	file_descriptors.k2_fd = k2;
	file_descriptors.k3_fd = k3;

	while(!stop_request){
		nr = epoll_wait(epfd, events, sizeof(events), 100);
		if (nr == -1){ /* error*/
			printf("Error : couldn't wait event in epoll context ! errno=%d Exiting \n", errno);
		}

		for(int i=0; i<nr; i++){
			//printf ("event=%ld on fd=%d\n", events[i].events, events[i].data.fd);

			void(*handler)( ) = events[i].data.ptr ;
			handler(file_descriptors);
		}
	}

	pthread_exit(NULL);
}

void freq_handler(struct file_desc file_descriptors){
	char buffer[11] = {'\0'};
	int nb = pread(file_descriptors.freq_fd, &buffer, 11, 0);
	cur_freq = atoi(buffer);
	pthread_mutex_lock(&lcd_mutex_lock);
	lcd_write_freq(cur_freq);
	pthread_mutex_unlock(&lcd_mutex_lock);
}

void cpu_temp_handler(struct file_desc file_descriptors){
	char buffer[7] = {'\0'};
	int nb = pread(file_descriptors.cpu_temp_fd, &buffer, 7, 0);
	cur_cpu_temp = atoi(buffer);
	pthread_mutex_lock(&lcd_mutex_lock);
	lcd_write_temp(cur_cpu_temp);
	pthread_mutex_unlock(&lcd_mutex_lock);
}

void timer_handler(struct file_desc file_descriptors){
	reset_timer(file_descriptors);
	printf("Timer event\n");
	
	// Turn power led of
	pwrite (file_descriptors.led_pwr_fd, "0", sizeof("0"), 0);
	
}

void k1_handler(struct file_desc file_descriptors){
	char value = -1;
	pread(file_descriptors.k1_fd, &value, 1, 0);
	if((value & 0x01) == 1){
		printf("K1 pressed\n");

		// Action	
		set_timer(file_descriptors);

		pwrite (file_descriptors.led_pwr_fd, "1", sizeof("1"), 0);
		
		increment_freq();		
		
	} else {
		printf("K1 released\n");
	}			
}

void k2_handler(struct file_desc file_descriptors){
	char value = -1;
	pread(file_descriptors.k2_fd, &value, 1, 0);
	if((value & 0x01) == 1){
		printf("K2 pressed\n");

		// Action
		set_timer(file_descriptors);

		pwrite (file_descriptors.led_pwr_fd, "1", sizeof("1"), 0);

		decrement_freq();

	} else {
		printf("K2 released\n");
	}	
}

void k3_handler(struct file_desc file_descriptors){
	char value = -1;
	pread(file_descriptors.k3_fd, &value, 1, 0);
	if((value & 0x01) == 1){
		printf("K3 pressed\n");
		
		// Action
		set_timer(file_descriptors);

		pwrite (file_descriptors.led_pwr_fd, "1", sizeof("1"), 0);

		if(cur_mode == 0){
			cur_mode = 1;
		} else {
			cur_mode = 0;
		}

		set_mode(cur_mode);
		pthread_mutex_lock(&lcd_mutex_lock);
		lcd_write_mode(cur_mode);
		pthread_mutex_unlock(&lcd_mutex_lock);

	} else {
		printf("K3 released\n");
	}			
}

void set_timer(struct file_desc file_descriptors){
	ts.it_value.tv_nsec = 0;
	ts.it_value.tv_sec = 1;
	if (timerfd_settime(file_descriptors.timer_fd, 0, &ts, NULL) < 0) {
		printf("timerfd_settime() failed: errno=%d -- B\n", errno);	
		close(file_descriptors.timer_fd);	
	}
	
}

void reset_timer(struct file_desc file_descriptors){
	ts.it_value.tv_nsec = 0;
	ts.it_value.tv_sec = 0;
	if (timerfd_settime(file_descriptors.timer_fd, 0, &ts, NULL) < 0) {
		printf("timerfd_settime() failed: errno=%d -- B\n", errno);	
		close(file_descriptors.timer_fd);	
	}
	
}


void increment_freq(){
	if(cur_mode == 1){
		if(cur_freq < 10){
			set_freq(cur_freq + 1);
		} else if(cur_freq < 30){
			set_freq(cur_freq + 5);
		} else if(cur_freq < 100){
			set_freq(cur_freq + 10);
		} else {
			set_freq(cur_freq + 50);
		}
	} else {
		printf("Cannot change freq in auto mode\n");
	}
}

void decrement_freq(){
	if(cur_mode == 1){
		if(cur_freq > 100){
			set_freq(cur_freq - 50);
		} else if(cur_freq > 30){
			set_freq(cur_freq - 10);
		} else if(cur_freq > 10){
			set_freq(cur_freq - 5);
		} else if (cur_freq > 0) {
			set_freq(cur_freq - 1);
		}
	} else {
		printf("Cannot change freq in auto mode\n");
	}		
}
