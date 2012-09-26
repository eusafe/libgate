/*
 *
 * Adapter for controller of gate
 * 
 * Designed by Evgeny Byrganov <eu dot safeschool at gmail dot com> for safeschool.ru, 2012
 *  
 * $Id: gp_util.c 2737 2012-09-26 08:34:54Z eu $
 *
 */


#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#include "libgate.h"

uint8_t crc8_xor(uint8_t* p, int l) {
	uint8_t r=*p;
	
	while ( l > 1 ) {
		r ^= *(++p ); l--;
	}
	
	return r;
}

int memcpy_esc(uint8_t* d, uint8_t* s, int l) {
	int i; int c=0;
	for(i=0;i<l;i++) {
		if ((*s & 0xF0) == 0xF0) {
		    switch(*s) {
		  	case GP_INIT:
		  		c++;
		  		*d++=GP_ESC;
		  		*d=1;
		  		break;
		  	case GP_END:
		  		c++;
		  		*d++=GP_ESC;
		  		*d=2;
		  		break;
		  	case GP_ESC:
		  		c++;
		  		*d++=GP_ESC;
		  		*d=3;
		  		break;
		  	default:
		  		*d=*s;
		    }
		} else {
	  		*d=*s;
		}
		d++; s++; c++;
	}
	*d='\0';
	return c;
}

// can  d = s
int memcpy_unesc(uint8_t* d, uint8_t* s, int len) {
	int i;
	int l=len;
	for(i=0;i<len;i++) {
		if ( *s  == GP_ESC) {
		    l--;
		    switch(*(++s)) {
		  	case 1:
		  		*d=GP_INIT;
		  		break;
		  	case 2:
		  		*d=GP_END;
		  		break;
		  	case 3:
		  		*d=GP_ESC;
		  		break;
		  	default:
		  		return 0;
		    }
		} else {
	  		*d=*s;
		}
		d++; s++;;
	}
	*d='\0';
	return l;
}

// http://www.chesterproductions.net.nz/blogs/it/code/rewriting-the-wheel-bin2hex-hex2bin/34/
int bin2hex(uint8_t* d ,uint8_t* s, size_t len, int revert) {
	int i; int c=0;
	if(revert == 1) {
		s +=len-1;
		for(i=0;i<len;i++,s--) {
			uint8_t h=*s >> 4;
			uint8_t l=*s & 0x0f;
//			printf("%x %x\n", h, l);
			*(d++)=(h < 10 )? h + '0': h +'A'-10;
			*(d++)=(l < 10 )? l + '0': l +'A'-10;
			c++;
		}
	} else {
		for(i=0;i<len;i++,s++) {
			uint8_t h=*s >> 4;
			uint8_t l=*s & 0x0f;
//			printf("%x %x\n", h, l);
			*(d++)=(h < 10 )? h + '0': h +'A'-10;
			*(d++)=(l < 10 )? l + '0': l +'A'-10;
			c++;
		}
	}
	*d='\0';
	return 2*c;
}


int long2hex(uint8_t* d, uint64_t a) {
	int i; int c=0;
	int l = sizeof(a);
	uint8_t* s=(uint8_t*)&a+l-1;
//		s +=len-1;
	for(i=0;i<l;i++,s--) {
		uint8_t h=*s >> 4;
		uint8_t l=*s & 0x0f;
//		printf("%x %x\n", h, l);
		*(d++)=(h < 10 )? h + '0': h +'A'-10;
		*(d++)=(l < 10 )? l + '0': l +'A'-10;
		c+=2;
	}
/*	for(i=0;i<l;i++,s++) {
		uint8_t h=*s >> 4;
		uint8_t l=*s & 0x0f;
		printf("%x %x\n", h, l);
		*(d++)=(h < 10 )? h + '0': h +'A'-10;
		*(d++)=(l < 10 )? l + '0': l +'A'-10;
		c++;
	}*/
	*d='\0';
	return c;
}

int hex2long() {
	return 1;
}

uint64_t bin2int (uint8_t* s, size_t len, int revert) {
	uint64_t a=0;
	int l=(len <= sizeof a)? len : sizeof  a;
	memcpy((uint8_t*)&a,s, l);
	if(revert == 1) 
		a=be64toh(a) >> 8 * (8 - l);
	
	return a;
}

void  int2bin(uint8_t* d, uint64_t a, size_t len, int revert) {
	if(revert == 1) 
		a=be64toh(a) >> 8 * (8 - len);
	
	memcpy(d,(uint8_t*)&a, len);
}

/*
uint64_t gp_dev_bvector() {
	uint64_t v = (uint64_t)0;
	uint64_t b = 1;
	int i;
	
	for(i=1;i<=gp_cfg.max_dev_n;i++) {
		if( devices[i].activ == 1 ) v |= b;
		b <<= 1;
//		fprintf(stderr, "vervtor: v:%x, b:%x\n",v,b);
//		devices[i].timeout_count=0;
	}
	return v;
}

char* gp_dev_vector() {
	static char v[256*2];
	int i=0, gp_dst;
	for(gp_dst=1;gp_dst<=gp_cfg.max_dev_n;i++,gp_dst++) {
		if( devices[gp_dst].activ == 1 ) 
			v[i] = '0' + gp_dst % 10;
		else
			v[i] = '.';
		if( gp_dst % 4 == 0 ) {
			i++;
			v[i] = ' ';
		}
	}
	v[i]='\0';
	return v;
}

*/

static struct tm curtime;

int set_rtc_date(int dev) {
/*	gp_date_rtc_t d  = { 0, 0,  0x13, 0x15, 2, 0x14, 0x08, 0x12, 0x10 }; 
	ad_set_date(AD_Q_SECOND, dev, &d);
	return;*/
	gp_date_rtc_t d;
	time_t z;
	time(&z);
	struct tm* t=localtime_r(&z,&curtime);
	
		d.year = int2bcd(t->tm_year%100);
		d.mon = int2bcd(t->tm_mon+1);
		d.day = int2bcd(t->tm_mday);
		d.wday = int2bcd((t->tm_wday)? t->tm_wday:7);
		d.hour = int2bcd(t->tm_hour);
		d.min = int2bcd(t->tm_min);
		d.sec = int2bcd(t->tm_sec);
		d.ctrl = 0x10;
/*	{
	gp_date_rtc_t *t=&d;
	fprintf(stderr, "RTC date: %02x-%02x-%02x %02x:%02x:%02x  (%s)\n", 
				t->year, t->mon, t->day, t->hour , t->min, t->sec, (t->stop)? "stopped":"working" );
	}*/
	return 	ad_set_date(AD_Q_SECOND, dev, &d);
}


struct tm* get_rtc_date(gp_date_rtc_t* d) {
	curtime.tm_sec = bcd2int(d->sec);
	curtime.tm_min = bcd2int(d->min);
	curtime.tm_hour = bcd2int(d->hour);
	curtime.tm_wday = bcd2int(d->wday)%7;
	curtime.tm_mday = bcd2int(d->day);
	curtime.tm_mon = bcd2int(d->mon)-1;
	curtime.tm_year = bcd2int(d->year)+100;
	
	return &curtime;
}

struct tm* get_ev_date(gp_event_t* d) {
	curtime.tm_sec = bcd2int(d->sec);
	curtime.tm_min = bcd2int(d->min);
	curtime.tm_hour = bcd2int(d->hour);
	curtime.tm_mday = bcd2int(d->day);
	curtime.tm_mon = bcd2int(d->mon)-1;
	
	return &curtime;
}

time_t get_ev_time(gp_event_t* d) {
	return mktime(get_ev_date(d));
}

