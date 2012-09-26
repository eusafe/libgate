
char *version = "$Id: adapter.c 2737 2012-09-26 08:34:54Z eu $ (gate 0.5)";

#include <event.h>
#include <signal.h>
#include <time.h>

#include <sys/queue.h>
#include <sys/stat.h>
#include <sys/param.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <err.h>
#include <syslog.h>
#include <stdarg.h>
#include <ctype.h>

#include <assert.h>
#include <semaphore.h>
#include <pthread.h>

#include <regex.h>

#include "libgate.h"
//#include "gp_layer.h"
//#include "ad_target.h"

struct event evkeep;
// how often to send keep-alive packet to controller? (seconds)
volatile int keepalive_timer = 11;
struct timeval keep_tv = { 5, 0 };

// PID file descriptor
//volatile int pidfd = -1;

//int my_dst=7;


// is switch socket an INET socket (or otherwise is it a unix file socket?)
//volatile int network = 0;
// switch socket path
volatile char swsock_path[MAXPATHLEN];

// uint8_t activ_dev[] = { 2, 7, 255 };
uint8_t activ_dev[] = { 7, 3, 1 };
int background=0;
int debug = 7;
int verbose = 1;
//extern int mydev;
// debug logging
void zprintf (int lvl, const char *fmt, ...) {
	va_list ap;
	int priority;
	int sys=0;
	int std=verbose;
	char pref[1024]="";
	
	if ( lvl > debug ) return;
	switch(lvl) {
		case 1:
			sys=1;
			priority=LOG_CRIT;
			strcpy(pref,"I1: ");
			break;
		case 2:
			sys=1;
			priority=LOG_ERR;
			strcpy(pref,"I2: ");
			break;
		case 3:
			sys=1;
			priority=LOG_WARNING;
			strcpy(pref,"I3: ");
			break;
		case 4:
			sys=1;
		case 5:
			priority=LOG_NOTICE;
			strcpy(pref,"I4: ");
			break;
		case 6:
			sys=1;
		case 7:
			priority=LOG_INFO;
			strcpy(pref,"I5: ");
			break;
		case 8:
		case 9:
			priority=LOG_DEBUG;
			strcpy(pref,"I6: ");
			break;
		default:
			return;
	}
	strncat(pref,fmt,1000);
	if( sys ) {
		va_start(ap, pref);
		vsyslog(priority, pref, ap);
		va_end(ap);
	}
	if( std && !background ) {
		va_start(ap, fmt);
		vfprintf(stderr, fmt, ap);
		va_end(ap);
	}
	return;
}

// syslog initialize
void log_init (char* module,char* version) {
	openlog(module, LOG_PID | LOG_NDELAY | LOG_CONS, LOG_DAEMON);
//	syslog(LOG_INFO, "%s started", version);
	zprintf(4, "%s started\n", version);
	return;
}
// standard signals handler
void signal_handler(int sig) {
	int rc;
	switch(sig) {
		case SIGHUP:
			zprintf(4, "SIGHUP: initializing adapter...\n");
/*			rc = adapter_init();
			if (rc < 0) {
				dprint(DLS, "SIGHUP: failed!\n");
				// daemon_exit(1); // never exit on error!
			} else
				dprint(DLS, "SIGHUP: init successful\n");*/
			break;
		case SIGINT:
		case SIGTERM:
			zprintf(1, "SIGTERM: exiting...");
		 	db_close();
			exit(0);
//			daemon_exit(0);
			break;
		case SIGPIPE:
			// artimashev: my guess is that SIGPIPE is never used here,
			//	so it must be safe to remove this signal from here at all
			zprintf(1, "caught SIGPIPE from main thread");
			break;
	}
	return;
}

void signals_init (void) {
	zprintf(1, "set signals\n");
	// setup signal handlers
	if (signal(SIGTSTP,SIG_IGN) == SIG_ERR) err(1, "signal SIGTSTP");
	if (signal(SIGTTOU,SIG_IGN) == SIG_ERR) err(1, "signal SIGTTOU");
	if (signal(SIGTTIN,SIG_IGN) == SIG_ERR) err(1, "signal SIGTTIN");
	if (signal(SIGCHLD,SIG_IGN) == SIG_ERR) err(1, "signal SIGCHLD");
	if (signal(SIGPIPE,signal_handler) == SIG_ERR) err(1, "signal SIGPIPE");
	if (signal(SIGHUP, signal_handler) == SIG_ERR) err(1, "signal SIGHUP");
	if (signal(SIGTERM,signal_handler) == SIG_ERR) err(1, "signal SIGTERM");
}

void keepalive_port(int fd, short event, void *arg) {
	//struct event *ev = arg;
	// controller keep-alive command
	static int nc=1;
	
	int dst=0;
/*	if( (nc/5)%2 ) 
		ad_turn_on(AD_Q_SECOND, activ_dev[dst],nc%2);
	else 
		ad_turn_off(AD_Q_SECOND, activ_dev[dst]);*/
	
	ad_get_date(AD_Q_SECOND, activ_dev[dst]);
	
 	/* Reschedule this event */
	if ( event_add(&evkeep, &keep_tv) < 0) syslog(LOG_ERR, "event_add.evkeep setup: %m");

}



int main() {
 	
//	start_log("adapter",version);
	
	log_init("adapter",version);

// 	sem_init(&available_sem, 0, 0);
 	
	gp_queue_init();
	
	/* Initalize the event library */
	event_init();
	
	db_init("/tmp/gp_cards");
	
	gp_init();
	
	sw_init ();sw_proc_init();
	
	event_set(&evkeep, -1 , EV_TIMEOUT|EV_PERSIST, keepalive_port, (void*)&evkeep);
//	evtimer_set(&evkeep,keepalive_port,&evkeep);
 	evutil_timerclear(&keep_tv);
	keep_tv.tv_sec =  keepalive_timer;
// 	if ( event_add(&evkeep, &keep_tv) < 0) syslog(LOG_ERR, "event_add.evkeep setup: %m");

// 	fprintf(stderr, "mask: 0x%08llX, [%s]\n",  gp_dev_bvector(),gp_dev_vector());
// ad_get_info(0);
//	ad_exchange(AD_Q_SECOND, 2, 5);
//	ad_exchange(AD_Q_SECOND, 3, 4);
	
	int dst=0;
//	gp_times_t times = { 150, 200, 0, 0 };
	for(dst=0;dst<sizeof(activ_dev);dst++) {
		int dev=activ_dev[dst];
//		ad_reset_token_bound(AD_Q_SECOND, dev);
 		ad_reset_buff(AD_Q_SECOND, dev );
// HACK to gp_dev_init
//		ad_set_times(AD_Q_SECOND, dev, &times);
//		set_rtc_date(dev);
//		get_rtc_date(dev);
 	}

	for(dst=0;dst<sizeof(activ_dev);dst++) {
		int dev=activ_dev[dst];
		
		ad_get_times(AD_Q_SECOND, dev );
//		ad_get_token_bound(AD_Q_SECOND, dev);
	}


//	ad_turn_on(AD_Q_SECOND, my_dst,1);
//	ad_get_buff_addr(AD_Q_SECOND, my_dst);
//	fprintf(stderr, "Begin event dispatch\n\n");
	
	zprintf(3, "Begin event dispatch\n\n");
	event_dispatch();
 	db_close();
 return 0;
}
