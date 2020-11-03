/**
 * Autĥor:	Isaia Spinelli
 * Date:	23.10.2020
 * Utilisation de Select via le driver /Pilote_ex2/ex7_Op_Bloq
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

/*
 * status led - gpioa.10 --> gpio10
 * power led  - gpiol.10 --> gpio362
 */
#define GPIO_EXPORT		"/sys/class/gpio/export"
#define GPIO_UNEXPORT	"/sys/class/gpio/unexport"
#define GPIO_LED		"/sys/class/gpio/gpio10"
#define LED				"10"


void toggleLed(int led);
void handleTimer(int duty, int led);

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
	// GET PARAMETS WITH ARGS 
	
	long duty = 50;		// 50 %
	int HZ = 2;	// 2 Hz
	int periodTimerMs = 500/HZ ; 
	
	if ((argc != 1) && (argc != 2) && (argc != 3)) {
	   fprintf(stderr, "%s [ Hz(1-1000) duty-cylce (1-99)]\n",
			   argv[0]);
	   exit(EXIT_FAILURE);
   }
   if (argc > 1) {
	   HZ = atoi(argv[1]);
	   periodTimerMs = 500/HZ ; 
	   
	   if (argc > 2) {
		   duty = atoi(argv[2]);
	   }
   }
	
	long periodTimerNs = periodTimerMs * 1000000; // in ns
	periodTimerNs /= 100; // Pour gérer le duty cycle
	
	printf("Hz = %d - duty = %ld\n", HZ, duty );


	// OPEN GREEN LED 
 	int led = open_led();
	pwrite (led, "1", sizeof("1"), 0);


	// INIT TIMER 
	
	struct itimerspec ts;
	struct timespec now;
	
	int tfd = timerfd_create(CLOCK_REALTIME, 0);
	if (tfd == -1) {
		printf("timerfd_create() failed: errno=%d\n", errno);
		return EXIT_FAILURE;
	}
	
	if (clock_gettime(CLOCK_REALTIME, &now) == -1)
		printf("Error clock gettime \n");
		
	ts.it_value.tv_sec =  now.tv_sec ;
	ts.it_value.tv_nsec = now.tv_nsec; 
	ts.it_interval.tv_sec = periodTimerNs / 1000000000;
	ts.it_interval.tv_nsec = (periodTimerNs % 1000000000);

	if (timerfd_settime(tfd, TFD_TIMER_ABSTIME, &ts, NULL) < 0) {
		printf("timerfd_settime() failed: errno=%d\n", errno);
		close(tfd);
		return EXIT_FAILURE;
	}
	
	
	// INIT EPOLL
	int epfd = epoll_create(1);
	if (epfd == -1) {
		printf("epoll_create() failed: errno=%d\n", errno);
		close(tfd);
		return EXIT_FAILURE;
	}

	struct epoll_event ev;
	ev.events = EPOLLIN;
	ev.data.fd = tfd;
	if (epoll_ctl(epfd, EPOLL_CTL_ADD, tfd, &ev) == -1) {
		printf("epoll_ctl(ADD) failed: errno=%d\n", errno);
		close(epfd);
		close(tfd);
		return EXIT_FAILURE;
	}
	
	
	struct epoll_event events[2];
	int nr;

	while(1) {
		
		// WAIT EVENT
		nr = epoll_wait(epfd, events, 1, -1);
	
		if (nr < 0) {
			printf("epoll_wait() failed: errno=%d\n", errno);
			close(epfd);
			close(tfd);
			return EXIT_FAILURE;
		}
		
		// HANDLE EVENTS 
		for (int i=0; i<nr; i++) {
			// TIMER
			if (events[i].data.fd == tfd){
				uint64_t value;
				read(tfd, &value, 8);
				handleTimer(duty, led);
			}
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


void handleTimer(int duty, int led){
	static int count = 0;
	
	//printf("%d,", count);
			
	count++;
	
	if (count == duty) {
		toggleLed(led);
		//printf(" tog\n");
		//printf ("event=%d on fd=%d\n", events[i].events, events[i].data.fd);
	}
	
	if (count >= 100){
		toggleLed(led);
		count = 0;
		//printf("\n");
	}
	
}
