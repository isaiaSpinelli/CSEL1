/**
 * Autĥor:	Isaia Spinelli
 * Date:	06.11.2020
 * toggle led.
 * Period & Duty cycle possible in args  [ periode[ms] / duty-cylce[%] (default: 250ms / 50 %)
 * Example : ./app 500 10
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

#include <stdio.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <string.h>

// Structure sharded
struct my_struct {
	int duty;
	int led_fd;
	int k1_fd;
	int k2_fd;
	int k3_fd;
	int timer_fd;
	
};

long periodTimerMs = 250; // 2Hz


/*
 * status led - gpioa.10 --> gpio10
 * power led  - gpiol.10 --> gpio362
 */
#define GPIO_EXPORT		"/sys/class/gpio/export"
#define GPIO_UNEXPORT	"/sys/class/gpio/unexport"
#define GPIO_LED		"/sys/class/gpio/gpio10"
#define GPIO_PATH		"/sys/class/gpio/gpio"
#define LED				"10"
#define BUTTON_K1		"0"
#define BUTTON_K2		"2"
#define BUTTON_K3		"3"


void toggleLed(int led);
// handler
void handleTimer_1(struct my_struct my_data);
void handleK1(struct my_struct my_data);
void handleK2(struct my_struct my_data);
void handleK3(struct my_struct my_data);

static int open_gpio(const char* GPIO);
int setupTimeTimer(int tfd, long periodTimerMs);

static int open_led()
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
	f = open (GPIO_LED "/direction", O_WRONLY);
	write (f, "out", 3);
	close (f);

	// open gpio value attribute
 	f = open (GPIO_LED "/value", O_RDWR);
	return f;
}

int main(int argc, char* argv[]) 
{
	int duty = 50;		// 50 %

	// GET PARAMETS WITH ARGS 
	if (argc == 1) {
		fprintf(stdout, "\tusage : %s [ periode[ms] / duty-cylce[%%]\n",
			   argv[0]);
	} else if ((argc != 2) && (argc != 3)) {
	   fprintf(stderr, "%s [ periode[ms] / duty-cylce[%%] ]\n",
			   argv[0]);
	   exit(EXIT_FAILURE);
   }
   if (argc > 1) {
	   periodTimerMs = atoi(argv[1]);
	   
	   if (argc > 2) {
		   duty = atoi(argv[2]);
	   }
   }
	
	printf("Duty = %d\n", duty );


	// OPEN GREEN LED 
 	int led = open_led();
	pwrite (led, "1", sizeof("1"), 0);
	
	// OPEN GPIO 0 (K1)
	int K1 = open_gpio(BUTTON_K1);
	
	// OPEN GPIO 2 (K2)
	int K2 = open_gpio(BUTTON_K2);
	
	// OPEN GPIO 3 (K3)
	int K3 = open_gpio(BUTTON_K3);

	// INIT TIMER 
	int tfd = timerfd_create(CLOCK_REALTIME, 0);
	if (tfd == -1) {
		printf("timerfd_create() failed: errno=%d\n", errno);
		return EXIT_FAILURE;
	}
	if ( setupTimeTimer(tfd, periodTimerMs) == EXIT_FAILURE) {
		printf("setupTimeTimerfailed: errno=%d\n", errno);
		close(tfd);
		return EXIT_FAILURE;
	}
	
	// INIT EPOLL
	int epfd = epoll_create1(0);
	if (epfd == -1) {
		printf("epoll_create() failed: errno=%d\n", errno);
		close(tfd);
		return EXIT_FAILURE;
	}
	// ADD POLL TIMER
	struct epoll_event ev;
	ev.events = EPOLLIN;
	ev.data.ptr = handleTimer_1;
	if (epoll_ctl(epfd, EPOLL_CTL_ADD, tfd, &ev) == -1) {
		printf("epoll_ctl(ADD) failed: errno=%d\n", errno);
		close(epfd);
		close(tfd);
		close(K1);
		return EXIT_FAILURE;
	}
	// ADD POLL K1
	struct epoll_event ev_K1;
	ev_K1.events = EPOLLERR;
	ev_K1.data.ptr = handleK1;
	if (epoll_ctl(epfd, EPOLL_CTL_ADD, K1, &ev_K1) == -1) {
		printf("epoll_ctl(ADD) failed: errno=%d\n", errno);
		close(epfd);
		close(tfd);
		close(K1);
		return EXIT_FAILURE;
	}
	// ADD POLL K2
	struct epoll_event ev_K2;
	ev_K2.events = EPOLLERR;
	ev_K2.data.ptr = handleK2;
	if (epoll_ctl(epfd, EPOLL_CTL_ADD, K2, &ev_K2) == -1) {
		printf("epoll_ctl(ADD) failed: errno=%d\n", errno);
		close(epfd);
		close(tfd);
		close(K2);
		return EXIT_FAILURE;
	}
	// ADD POLL K3
	struct epoll_event ev_K3;
	ev_K3.events = EPOLLERR;
	ev_K3.data.ptr = handleK3;
	if (epoll_ctl(epfd, EPOLL_CTL_ADD, K3, &ev_K3) == -1) {
		printf("epoll_ctl(ADD) failed: errno=%d\n", errno);
		close(epfd);
		close(tfd);
		close(K3);
		return EXIT_FAILURE;
	}
	
	
	struct epoll_event events[4];
	int nr;
	
	// prepare struct
	struct my_struct my_data;
	my_data.duty = duty;
	my_data.led_fd = led;
	my_data.k1_fd = K1;
	my_data.k2_fd = K2;
	my_data.k3_fd = K3;
	my_data.timer_fd = tfd;

	while(1) {
		
		// WAIT EVENT
		nr = epoll_wait(epfd, events, 4, -1);
		if (nr < 0) {
			printf("epoll_wait() failed: errno=%d\n", errno);
			close(epfd);
			close(tfd);
			return EXIT_FAILURE;
		}
		
		// Get value of K1 (for pressed in continue)
		/*char K1Val[3];
		read(K1, K1Val, 2);
		int valueK1 = atoi(K1Val);
		printf("val = %d\n", valueK1);*/
		
		// HANDLE EVENTS 
		for (int i=0; i<nr; i++) {
			
			void(*handler)( ) = events[i].data.ptr ;
			handler(my_data);
      
		}
	}

	return 0;
}


void toggleLed(int led){
	static int k = 0;
	
	k = (k+1)%2;
	if (k == 0) 
		pwrite (led, "1", sizeof("1"), 0);
	else
		pwrite (led, "0", sizeof("0"), 0);
				
}

static int open_gpio(const char* GPIO)
{
	char path[100];
	char pathGPIO[100];
	strcpy(pathGPIO, "/sys/class/gpio/gpio" );
	strcat(pathGPIO, GPIO);
	
	// unexport pin out of sysfs (reinitialization)
	int f = open (GPIO_UNEXPORT, O_WRONLY);
	write (f, GPIO, strlen(GPIO));
	close (f);

	// export pin to sysfs
	f = open (GPIO_EXPORT, O_WRONLY);
	write (f, GPIO, strlen(GPIO));
	close (f);	

	// config direction
	strcpy(path, pathGPIO);
	strcat(path, "/direction");
	f = open (path, O_WRONLY);
	write (f, "in", 3);
	close (f);
	
	// config edge
	strcpy(path, pathGPIO);
	strcat(path, "/edge");
	f = open (path, O_WRONLY);
	write (f, "rising", 7);
	close (f);

	// open gpio value attribute
	strcpy(path, pathGPIO);
	strcat(path, "/value");
 	f = open (path , O_RDWR);
 	
 	uint64_t value;
	read(f, &value, 8);
	
	return f;
}

int setupTimeTimer(int tfd, long periodTimerMs){
	
	
	long periodTimerNs = periodTimerMs * 1000000; // in ns
	periodTimerNs /= 100; // Pour gérer le duty cycle
	
	printf("periodTimerMs = %ld\n", periodTimerMs );
	
	
	struct timespec now;
	if (clock_gettime(CLOCK_REALTIME, &now) == -1)
		printf("Error clock gettime \n");
	
	struct itimerspec ts;
	ts.it_value.tv_sec =  now.tv_sec ;
	ts.it_value.tv_nsec = now.tv_nsec; 
	ts.it_interval.tv_sec = periodTimerNs / 1000000000;
	ts.it_interval.tv_nsec = (periodTimerNs % 1000000000);

	if (timerfd_settime(tfd, TFD_TIMER_ABSTIME, &ts, NULL) < 0) {
		printf("timerfd_settime() failed: errno=%d\n", errno);
		close(tfd);
		return EXIT_FAILURE;
	}
	
	return EXIT_SUCCESS;
				
}

void handleTimer_1(struct my_struct my_data){
	static int count = 0;
	
	uint64_t value;
	read(my_data.timer_fd, &value, 8);
	
	count++;
	
	if (count == my_data.duty) {
		toggleLed(my_data.led_fd);
	}
	
	if (count >= 100){
		toggleLed(my_data.led_fd);
		count = 0;
	}
}

void handleK1(struct my_struct my_data){
	uint64_t value;
	read(my_data.k1_fd, &value, 8);
	
	if (periodTimerMs < 60000) {	// max 1 min
		periodTimerMs += 100;
	} else {
		printf("Period is max ! \n");
	}
				
	if ( setupTimeTimer(my_data.timer_fd, periodTimerMs) == EXIT_FAILURE) {
		printf("setupTimeTimerfailed: errno=%d\n", errno);
		close(my_data.timer_fd);
	}
}
void handleK2(struct my_struct my_data){
	uint64_t value;
	read(my_data.k2_fd, &value, 8);
	
	periodTimerMs = 250; // 2 Hz				
	if ( setupTimeTimer(my_data.timer_fd, periodTimerMs) == EXIT_FAILURE) {
		printf("setupTimeTimerfailed: errno=%d\n", errno);
		close(my_data.timer_fd);
	}
}
void handleK3(struct my_struct my_data){
	uint64_t value;
	read(my_data.k3_fd, &value, 8);
	
	if (periodTimerMs > 100) {
		periodTimerMs -= 100;
	} else {
		printf("Period is min ! \n");
	}
	if ( setupTimeTimer(my_data.timer_fd, periodTimerMs) == EXIT_FAILURE) {
		printf("setupTimeTimerfailed: errno=%d\n", errno);
		close(my_data.timer_fd);
	}
}
