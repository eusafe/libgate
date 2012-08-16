/*
 *
 * Adapter for controller of gate
 * 
 * Designed by Evgeny Byrganov <eu dot safeschool at gmail dot com> for safeschool.ru, 2012
 *  
 * $Id$
 *
 */

#include <termios.h>
#include <fcntl.h>

// #include <stdio.h>
// #include <stdarg.h>
// #include <stdint.h>
// #include <stdlib.h>
 


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
