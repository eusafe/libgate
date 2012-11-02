/*
 *
 * Adapter for controller of gate
 * 
 * Designed by Evgeny Byrganov <eu dot safeschool at gmail dot com> for safeschool.ru, 2012
 *  
 * $Id: gp_port.c 2772 2012-11-01 11:46:08Z eu $
 *
 */

#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

// #include <stdio.h>
// #include <stdarg.h>
// #include <stdint.h>
// #include <stdlib.h>

#include <syslog.h>
#include <time.h>
#include <sys/time.h>

#include "libgate.h"
int open_port(char* path, int speed);

void gp_close () {
	if (gp_cfg.fd >= 0) {
		zprintf(1, "closing controller port %m");
		if ( event_del(&gp_cfg.evport) < 0) syslog(LOG_ERR, "event_del (port): %m");
		close(gp_cfg.fd);
	}
	gp_cfg.fd = -1;
}

// try to reconnect to switch socket and/or main controller port if closed
int gp_reconnect (int ignore_timeout) {
	time_t now = time((time_t*)NULL);
	static time_t last_try=(time_t)0;
	static int portIsOpen=1; /* '1' Need for first open_port() call problem */

	if (ignore_timeout || (now - SOCKET_RETRY_TIMEOUT > last_try )) {
		last_try = now;
		if (gp_cfg.fd < 0) {
//			dprint(DL5, "trying to [re]connect to controller\n");
//	fprintf(stderr, "open_port: %s, %d \n",gp_cfg.port_path, gp_cfg.port_speed); 
//			if ((gp_cfg.fd = open_port((char*)gp_cfg.port_path, gp_cfg.port_speed, 1)) < 0) {
			if ((gp_cfg.fd = open_port((char*)gp_cfg.port_path, gp_cfg.port_speed)) < 0) {
				zprintf(1, "open port (errcode %d): %m", errno);
//				if (portIsOpen == 1) send_switch(GATEKEEPER_NUMERIC_ID | NAGIOS_NUMERIC_ID, ALERT_TTY_USB);
				portIsOpen=0;
			} else {
//				fprintf(stderr, "use port %s (%d)\n", gp_cfg.port_path, gp_cfg.fd);
//				if (adapter_init() < 0) syslog(LOG_CRIT, "cannot initialize adapter");
//				event_set(&evport, gp_cfg.fd, EV_READ|EV_PERSIST|EV_TIMEOUT, gp_receiv, (void*)&evport);
				event_set(&gp_cfg.evport, gp_cfg.fd, EV_READ|EV_TIMEOUT, gp_receiv, (void*)&gp_cfg.evport);
				if ( event_add(&gp_cfg.evport, &gp_cfg.timeout) < 0) syslog(LOG_ERR, "evport,event_add setup: %m");
				portIsOpen=1;
			}
		}
	}
	if( gp_cfg.fd < 0 ) 
		return 0;
	else
		return 1;
}


int open_port (char *path, int speed)
{
	int fd;
	struct termios opt;
	int flags=O_RDWR | O_NOCTTY | O_NDELAY;

	if ((fd = open(path, flags)) < 0)
		return (-1);

//	fcntl(fd, F_SETFL, FNDELAY);
	fcntl(fd, F_SETFL, 0);
	
	tcgetattr(fd, &opt);
	cfmakeraw(&opt);
	
	cfsetspeed(&opt, (speed_t)speed);
	
	opt.c_cc[VMIN]=128;
	opt.c_cc[VTIME]=1;

	// ~PARENB = parity disable; no parity bit is added to each character
	// ~CSTOPB = don't send 2 stop bits; use one stop bit
	// ~CSIZE = no character size mask
	// !CRTSCTS = no CTS flow control of output
	opt.c_cflag &= ~(PARENB | CSTOPB | CSIZE | CRTSCTS);

	// CS8 = 8 bits; This size does not include the parity bit, if any (see CSTOPB for parity bit info)
	// CLOCAL = ignore modem status lines (a connection does not depend on the state of the modem status lines)
	// CREAD = enable receiver; This flag would be omitted, if it were not part of the specification.
	opt.c_cflag |= (CS8 | CLOCAL | CREAD);
	
	// ICANON = canonicalize input lines
//	opt.c_lflag &= ~ICANON;
	
	// !OPOST = disable output processing
//	opt.c_oflag &= ~OPOST;
//	opt.c_oflag |= NLDLY;


	// !ECHO = disable echoing
	// !ECHOE = don't visually erase chars; If ICANON are set, the ERASE character causes the terminal to erase the last character in the current line from the display, if possible.
	// !ECHOK = don't discard the current line (and echo the `\n' character) after the KILL character
	// !ECHOKE = don't visual erase for line kill 
	// !ECHOPRT = don't visual erase for hardcopy
	// !ECHOCTL = don't echo control chars as ^(Char)
	// !ECHONL = don't echo NL
	// !ISIG = disable signals INTR QUIT [D]SUSP
	// !IEXTEN = disable DISCARD and LNEXT
	//opt.c_lflag &= ~(ECHO | ECHOE | ECHOK | ECHOKE | ECHOPRT | ECHOCTL | ECHONL | ISIG | CRTSCTS | IEXTEN);
//	opt.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG | CRTSCTS);
//	opt.c_lflag |= ICANON;
	
//	opt.c_iflag |= ( IGNBRK |IGNCR );

	// ICRNL = don't map CR to NL
	//opt.c_iflag &= ~(ICRNL);
	// IGNBRK = ignore BREAK condition
	// IGNPAR = ignore (discard) parity errors
	// IGNCR = ignore CR
//	opt.c_iflag &= ~(INLCR | IGNCR | ICRNL | BRKINT | IGNBRK | ISTRIP | IXON | PARMRK);

	tcflush(fd,TCIFLUSH);
	tcsetattr(fd,TCSANOW,&opt);

	return (fd);
}


// //int stub_main() {
// int main() {
// 	return open_port("/dev/ttyUSB0",9600);
// }
