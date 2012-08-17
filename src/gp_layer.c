/*
 *
 * Adapter for controller of gate
 * 
 * Designed by Evgeny Byrganov <eu dot safeschool at gmail dot com> for safeschool.ru, 2012
 *  
 * $Id: gp_layer.c 2692 2012-08-17 13:36:51Z eu $
 *
 */

#include <event.h>
#include <sys/stat.h>
#include <sys/param.h>

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <errno.h>
#include <string.h>

#include <time.h>
#include <sys/time.h>

#include <termios.h>
#include <fcntl.h>

int open_port(char* path, int speed);

#include "libgate.h"

struct event evport;

// timestamp of last try of reconnection to switch socket and controller port (if one of them or both are not opened)
volatile time_t last_try;

struct {
	int fd;
	int port_speed;
	char port_path[MAXPATHLEN];
//	struct event ev_timeout;
	struct timeval timeout;
	struct event ev_scan;
	struct timeval scan_dev_interval;
	int gp_timeout;  /* in mil sec */
	uint32_t last_read;  /* last event in mil sec */
	int last_dev;
	int last_target;
	int polling;
	int max_dev_n;
	int max_timeout_count;
	int ev_block_size;
} gp_cfg = {
	.fd = -1,
	.port_path = "/dev/ttyUSB0",
	.port_speed = 19200,
	.timeout = { 0, 100000 },
	.gp_timeout = 100,
	.last_read = 0,
	.last_dev = 0,
	.polling = 0,
	.max_dev_n = 8,
	.max_timeout_count = 12,
	.ev_block_size = 8
};

#pragma pack(push,1)
struct {
	int len;
	int activ;
	uint32_t sent_time;  /* sent time in mil sec */
	int dev;
	int bank;
	int polling;
	union {
		uint8_t buf[GP_PORT_SEND_BUF_LENGTH];
		struct {
			uint8_t fl_begin;
			uint8_t id_ctrl;
			uint8_t dst;
			uint8_t cmd_n;
			uint8_t cmd_buff[120];
			uint8_t crc_stub ;
			uint8_t fl_end_stub;
		};
	};
} send_mess;
#pragma pack(pop)

typedef struct gp_dev {
	int activ;
	int timeout_count;
	int cid_revert;
	uint16_t bound_up;
	uint16_t bound_down;
	uint16_t bound_tocken;
	uint8_t last_cid[20];
	uint8_t last_cid_n[8];
	int last_ev;
	int new_cid_flag;
} gp_dev;
gp_dev  devices[256];

void gp_close ();

void print_buff(char*  mess, uint8_t* buff, int n) {	
	fprintf(stderr, mess);
	int i=0; 
	for(i=0; i < n ;i++) {
		fprintf(stderr, "0x%02x, ", buff[i]);
	}
	fprintf(stderr, " got %d bytes\n", n);
}
// send message to port
int gp_resend () {
	int nwritten;
//	int i=0;

 	if(gp_cfg.fd < 3) exit (100+gp_cfg.fd);
	
	if( debug >= 5 || (debug >= 4 && gp_cfg.polling == 0)  ) {
		print_buff("port  sending: ", send_mess.buf, send_mess.len);
/*		fprintf(stderr, "port  sending: "); 
		for(i=0;i<send_mess.len;i++) {
			fprintf(stderr, "0x%02x, ", send_mess.buf[i]);
		}
	 	fprintf(stderr, "writing  %d bytes into port\n", send_mess.len);*/
 	}
 		
 	if ( event_add(&evport, &gp_cfg.timeout) < 0) syslog(LOG_ERR, "event_add.send_mess.ev_timeout setup: %m");
	nwritten = write(gp_cfg.fd, send_mess.buf, send_mess.len);
	
// 	struct timeval tv;
//  	gettimeofday(&tv,NULL); 
// 	if( debug >= 5 || (debug >= 4 && gp_cfg.polling == 0)  ) {
//  		fprintf(stderr, "wrote  %d bytes into port, time: %u.%06u, is pend %d\n", nwritten, tv.tv_sec,tv.tv_usec, 
//  			event_pending(&evport,EV_READ|EV_TIMEOUT, &gp_cfg.timeout));
//  	}

	if (nwritten == 0) {
		if (errno == EPIPE) {
			syslog(LOG_ERR, "recipient closed connection");
			gp_close();
			// daemon_exit(1); // never exit on error!
		}
	} else if (nwritten < 0) {
		syslog(LOG_CRIT, "write (%d): %m", errno);
		gp_close();
		// daemon_exit(1); // never exit on error!
	}
//	if ( event_add(&evport, NULL) < 0) syslog(LOG_ERR, "evport,event_add setup: %m");
	return nwritten;
}

// TODO  need esc after crc8_xor 
int gp_send(cmd_send_t* cmd) {
	size_t cmd_len=cmd->data_len;
//	gp_cfg.gp_timeout=cmd->set_timeout;
	
	evutil_timerclear(&gp_cfg.timeout);
	gp_cfg.timeout.tv_usec = cmd->set_timeout * 1000;
	gp_cfg.polling = cmd->polling;
	gp_cfg.last_target = cmd->target_n;
	
	memset(&send_mess.buf, 0, sizeof(send_mess.buf));
	send_mess.fl_begin = GP_INIT;
	send_mess.id_ctrl = GP_CTRL;
//	memcpy(&send_mess.dst, cmd, cmd_len +2);
	send_mess.dst = cmd->dst;
	send_mess.cmd_n = cmd->cmd_n;
	cmd->cmd_buff[cmd_len]=crc8_xor(&cmd->cmd_n, cmd_len + 1);
//	fprintf(stderr, "Buffer size: %d bytes, crc8: 0x%02x\n", cmd_len, cmd->cmd_buff[cmd_len]);
	size_t len_esc=memcpy_esc((uint8_t*)&send_mess.dst, &cmd->dst,cmd_len+3)+3;
//	fprintf(stderr, "Final size: %d bytes\n", len_esc);
	send_mess.buf[len_esc-1]=GP_END;
// saved len and fd	
	send_mess.len=len_esc;
	send_mess.activ=1;
	send_mess.dev = cmd->dst;
	send_mess.bank = cmd->bank;
	
	struct timeval tv;
 	gettimeofday(&tv,NULL);
 	send_mess.sent_time=(tv.tv_sec%100000)* 1000 + tv.tv_usec/1000;
	
	return gp_resend();
}

int gp_send_idle() {
	static int gp_dst=0;
	int activ=0;
	
	do {
		gp_dst++;
		if( gp_dst > gp_cfg.max_dev_n ) { gp_dst=0; return 0; }
//	fprintf(stderr, "Check gp_send_idle for %d (%d)\n",gp_dst,devices[gp_dst].timeout_count );
		if(devices[gp_dst].timeout_count > gp_cfg.max_timeout_count) {
			if( devices[gp_dst].activ > 0) fprintf(stderr, "Disabled dev %d\n", send_mess.dst);
			devices[gp_dst].activ=activ=0;
			continue;
		}
		activ=1;
	} while( activ == 0 );
//	fprintf(stderr, "Run gp_send_idle for %d (%d)\n",gp_dst,devices[gp_dst].timeout_count );
	
/*	cmd_send_t z = {
		.cmd_n = 0x06,
		.set_timeout = 50,
		.polling = 1,
		.data_len = 0
	};*/
	cmd_send_t z = {
		.target_n = AD_TARGET_GET_CID,
		.cmd_n = 0x04,
		.set_timeout = 100,
		.polling = 1,
		.data_len = 5,
		.cmd_buff = { 0x00, 0xD0, 6, 0x00, 0x0C }
	};
	
	z.dst=gp_dst;
//return 0;
	return gp_send(&z);
}

void gp_dev_get_logs() {
	int i=0, gp_dst;
	for(gp_dst=1;gp_dst<=gp_cfg.max_dev_n;i++,gp_dst++) {
		if( devices[gp_dst].activ == 1 ) {
			ad_get_token_bound(AD_Q_SECOND, gp_dst );
			ad_get_buff_addr(AD_Q_SECOND,gp_dst );
		}
	}
	return;
}


void gp_all_dev_enable() {
	int i;
 	fprintf(stderr, "Reset activ for all devs.\n");
// 	syslog(LOG_ERR, "Reset activ for all devs.");
	for(i=1;i<=255;i++) {
//		devices[i].activ=0;
		devices[i].timeout_count=0;
	}
}
void gp_all_dev_ev(int fd, short event, void *arg) {
	evutil_timerclear(&gp_cfg.scan_dev_interval);
	gp_cfg.scan_dev_interval.tv_sec=15;
 	if ( event_add(&gp_cfg.ev_scan, &gp_cfg.scan_dev_interval) < 0) syslog(LOG_ERR, "event_add.evkeep setup: %m");
 	gp_dev_get_logs();
 	gp_all_dev_enable();
 	
}

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


#pragma pack(push,1)
struct {
	int len;
	union {
		uint8_t buf[GP_PORT_READ_BUF_LENGTH];
		struct {
			uint8_t fl_begin;
			uint8_t id_main;
			uint8_t dst;
			uint8_t id_ctrl;
			uint8_t src;
			uint8_t cmd_n;
			union {
				uint8_t cmd_buff[120];
				struct {
					uint8_t replay;
					uint8_t was_cmd_n;
				};
				struct {
					uint8_t addr;
					uint8_t mem;
					uint8_t bad_cmd_n;
				};
			};
			uint8_t crc_stub;
			uint8_t fl_end_stub;
		};
	};
} receiv_mess;
#pragma pack(pop)

uint8_t* receiv_buf_p=0;

int process_port_input() {
//	static uint8_t last_cid[20]="";
	if( debug >= 5 || (debug >= 4 && gp_cfg.polling == 0)  ) {
		fprintf(stderr, "cmd_buff: "); 
		int i=0; for(i=0; i < receiv_mess.len ;i++) {
			fprintf(stderr, "0x%02x, ", receiv_mess.cmd_buff[i]);
		}
		fprintf(stderr, "cmd: %02x, len=%u\n", receiv_mess.cmd_n,receiv_mess.len);
 	}
//	printf( "%s", receiv_mess.cmd_buff);
 
 	if( receiv_mess.cmd_n == 0x09 ) {
 		gp_info_t t;
 		memcpy(&t,receiv_mess.cmd_buff,( (receiv_mess.len <= sizeof t )? receiv_mess.len : 4));
 		int if_type=(t.if_type_n)? t.if_type2:t.if_type1;
 		fprintf(stderr, "ver: 0x%02x, model: 0x%u, if_type: %u, release=%d\n", 
 			t.ver, t.model,if_type , t.release );
 		devices[gp_cfg.last_dev].cid_revert = (if_type ==0)? 1:0;
 		
 	} else if ( receiv_mess.cmd_n == 0x0A ) {
 		gp_port_status_t t;
 		memcpy(&t,receiv_mess.cmd_buff,( (receiv_mess.len <= sizeof t )? receiv_mess.len : 4));
 	} else if ( receiv_mess.cmd_n == 0x06 ) {
 		gp_reg_cid_t* t=(gp_reg_cid_t*)receiv_mess.cmd_buff;
		uint8_t cid[20];
 		bin2hex(cid,t->cid,6,devices[gp_cfg.last_dev].cid_revert);
 		if( devices[gp_cfg.last_dev].new_cid_flag == 1 || devices[gp_cfg.last_dev].last_ev != t->ev ) {
			fprintf(stderr, "cid: 0x%12s, addr: %d, ev: %02x, src: %d\n", cid,t->addr,t->ev, gp_cfg.last_dev);
			fprintf(stderr,"cid: 0x%12s dev=%d, ev: %02x\n", devices[gp_cfg.last_dev].last_cid, gp_cfg.last_dev,t->ev);
			syslog(LOG_ERR,"cid: 0x%12s dev=%d, ev: %02x",   devices[gp_cfg.last_dev].last_cid, gp_cfg.last_dev,t->ev);
		
			if( (t->ev & ~1 ) == 0x02 ) {
				gp_token_rec_t t2;
				memcpy(&t2,devices[gp_cfg.last_dev].last_cid_n,8);
				t2.attr=0; t2.time_zone_mask=0xFF;
				ad_set_token(AD_Q_SECOND, gp_cfg.last_dev, devices[gp_cfg.last_dev].bound_tocken, &t2,1);
				fprintf(stderr,"Writing  cid: 0x%12s dev=%d, .bound=0x%02X\n", 
					devices[gp_cfg.last_dev].last_cid, gp_cfg.last_dev, devices[gp_cfg.last_dev].bound_tocken);
				devices[gp_cfg.last_dev].bound_tocken+=8;
				ad_set_token_bound(AD_Q_SECOND, gp_cfg.last_dev, devices[gp_cfg.last_dev].bound_tocken);
				ad_turn_on(AD_Q_SECOND, gp_cfg.last_dev, t->ev & 1);
			}
		}
		devices[gp_cfg.last_dev].last_ev=t->ev;
		devices[gp_cfg.last_dev].new_cid_flag = 0;
		
 	} else if ( receiv_mess.cmd_n == 0x04 ) {
/*  		uint64_t cid=0;
 		memcpy(&cid,receiv_mess.cmd_buff,( (receiv_mess.len <= sizeof  cid)? receiv_mess.len : sizeof  cid));
  		fprintf(stderr, "cid: 0x%012llX\n",  htobe64(cid << 16));*/\
  		if( gp_cfg.last_target == AD_TARGET_GET_CID ){
			uint8_t t[20]= {'0','0','0','0'};
			int l=bin2hex(&t,&receiv_mess.cmd_buff,( (receiv_mess.len <= 6)? receiv_mess.len : 6), devices[gp_cfg.last_dev].cid_revert);
			if( memcmp(devices[gp_cfg.last_dev].last_cid,t,l) != 0 ) {
				memcpy(devices[gp_cfg.last_dev].last_cid,t,l);
				memcpy(devices[gp_cfg.last_dev].last_cid_n,receiv_mess.cmd_buff,l);
//				fprintf(stderr, "cid: [0x%12s] dev=%d\n", t, send_mess.dst);
				devices[gp_cfg.last_dev].new_cid_flag=1;
//				syslog(LOG_ERR,"cid: 0x%12s dev=%d (%d)", t, send_mess.dst, l);
			}
			return ad_get_regs(AD_Q_SHORT, receiv_mess.src);
  		} else if ( gp_cfg.last_target == AD_TARGET_GET_TOKEN_BOUND) {
			gp_addr1_t* t = (gp_addr1_t*)receiv_mess.cmd_buff;
 			uint16_t bound=be16toh(t->bound);
 			devices[gp_cfg.last_dev].bound_tocken=bound;
			fprintf(stderr, "%d: token_buff: bound=0x%04X\n", send_mess.bank, bound);
//			syslog(LOG_ERR, "%d: token_buff: bound=0x%04X\n", send_mess.bank, bound);
  		} else if ( gp_cfg.last_target == AD_TARGET_GET_BUFF_ADDR) {
			gp_addr2_t* t = (gp_addr2_t*)receiv_mess.cmd_buff;
 			uint16_t bound_down=be16toh(t->bound_down);		
 			uint16_t bound_up=be16toh(t->bound_up);
 			devices[gp_cfg.last_dev].bound_down=bound_down;
 			devices[gp_cfg.last_dev].bound_up=bound_up;
 			
 			size_t l = bound_up - bound_down;
 			size_t l2 = gp_cfg.ev_block_size*sizeof(gp_event_t);
 			
			fprintf(stderr, "addr_buff: down=0x%04X up=0x%04X, len=%d. sizeof=%d  \n", bound_up, bound_down,l, sizeof(gp_event_t));
//			syslog(LOG_ERR, "addr_buff: down=0x%04X up=0x%04X, len=%d \n", bound_up, bound_down,l );
			uint16_t a = bound_down;
			l=(l>l2)? l2:l;
			if(l != 0) return ad_get_buff_data(AD_Q_SHORT,receiv_mess.src,a,l);
  		} else if ( gp_cfg.last_target == AD_TARGET_GET_BUFF_DATA) {
/*			gp_get_event_t t;
			memcpy(&t,&receiv_mess.cmd_buff,8);
			fprintf(stderr, "ev: code=%x, addr=0x%04X, (%d) \n", t.ev_code, htobe16(t.bound), sizeof t);
			syslog(LOG_ERR, "ev: code=%x, addr=0x%04X \n", t.ev_code, htobe16(t.bound));*/
  			gp_event_t* t=&receiv_mess.cmd_buff;
  			gp_event_t* bound=devices[gp_cfg.last_dev].bound_down;
// 			memcpy(&t,&receiv_mess.cmd_buff,receiv_mess.len);
 			int n=receiv_mess.len/sizeof(gp_event_t);
 			int i;
  			for(i=0;i<n;i++,t++,bound++) {
				fprintf(stderr, "ev: code=%x, addr=0x%04X, 2012-%02x-%02x %02x:%02x:%02x (%lu), dev=%d \n", 
					t->ev_code, htobe16(t->addr), t->mon, t->day, t->hour , t->min, t->sec, get_ev_time(t), send_mess.dst);
				syslog(LOG_ERR, "ev: code=%x, addr=0x%04X, 2012-%02x-%02x %02x:%02x:%02x (%lu), dev=%d \n", 
					t->ev_code, htobe16(t->addr), t->mon, t->day, t->hour , t->min, t->sec,	get_ev_time(t),send_mess.dst);
//				fprintf(stderr, "ev time: %u\n", get_ev_time(t));
//				fprintf(stderr, "ev time: %u, date: %s\n", get_ev_time(t), asctime(get_ev_date(t)));
  			} 
//			fprintf(stderr, "ev: lastbound=0x%04X \n", bound);
  			return ad_set_buff_addr_down(AD_Q_SHORT,receiv_mess.src,bound);	
  		} else if ( gp_cfg.last_target == AD_TARGET_GET_TIMES) {
  			gp_times_t* t=(gp_times_t*)&receiv_mess.cmd_buff;
			fprintf(stderr, "%d: open_door=%u, ctrl_close=%u, ctrl_open=%u, timeout_confirm=%u\n", 
				send_mess.bank, t->open_door, t->ctrl_close, t->ctrl_open, t->timeout_confirm );
			syslog(LOG_ERR, "%d: open_door=%u, ctrl_close=%u, ctrl_open=%u, timeout_confirm=%u\n", 
				send_mess.bank, t->open_door, t->ctrl_close, t->ctrl_open, t->timeout_confirm );
  		} else if ( gp_cfg.last_target == AD_TARGET_GET_DATE) {
  			gp_date_rtc_t* t=(gp_times_t*)&receiv_mess.cmd_buff;
			fprintf(stderr, "RTC date: 20%02x-%02x-%02x %02x:%02x:%02x  (%s)\n",
				t->year, t->mon, t->day, t->hour , t->min, t->sec, (t->stop)? "stopped":"working" );
			fprintf(stderr, "RTC date: %s\n", asctime(get_rtc_date(t)));
			syslog(LOG_ERR, "RTC date: 20%02x-%02x-%02x %02x:%02x:%02x\n", 
				t->year, t->mon, t->day, t->hour , t->min, t->sec);
  			
  		}
	}
 	return 0;
}

//void gp_receiv(uint8_t* in, int nread) {
// short event=4;
int mydev=-1;

void gp_receiv(int fd, short event, void *arg) {
	int nread=0;
//	int last=0;
	struct timeval tv;
 	
 	gettimeofday(&tv,NULL);
 	uint32_t current_time=(tv.tv_sec%100000)* 1000 + tv.tv_usec/1000;
 	uint32_t expect_time=(gp_cfg.timeout.tv_sec%100000)*1000 + gp_cfg.timeout.tv_usec/1000;
/*	fprintf(stderr, "Got  event: 0x%X, %u.%06u (%u ms), sent time %u ms, current timeout: %u.%06u (%u ms)\n",
		event,
		tv.tv_sec,tv.tv_usec,
		current_time,
		send_mess.sent_time,
		gp_cfg.timeout.tv_sec,gp_cfg.timeout.tv_usec,
		expect_time
		);*/
//	char buf[PORT_READ_BUF_LENGTH];
	
//	gp_reconnect(1);

//	memset(buf, 0, sizeof(buf));
				
//	if ( event_del(&send_mess.ev_timeout) < 0) syslog(LOG_ERR, "event_del (send_mess.ev_timeout): %m");
//	event_set(&evsocket, sockfd, EV_READ|EV_PERSIST, socket_read, (void*)&evsocket);
//	if ( event_del(&evport) < 0) syslog(LOG_ERR, "event_del (send_mess.ev_timeout): %m");

/*	if( (event & EV_WRITE) != 0 ) {
//		gp_put_cmd();
		return;
	}*/
/*	evutil_timerclear(&gp_cfg.timeout);
	gp_cfg.timeout.tv_usec = gp_cfg.gp_timeout * 1000;
 	if ( event_add(&evport, &gp_cfg.timeout) < 0) syslog(LOG_ERR, "event_add.send_mess.ev_timeout setup: %m");*/
	
	if( (event & EV_READ) == 0 ) {
// HACK - we check real timeout, not timer
//		if( current_time >= expect_time ) {	 	
//				} else if(gp_cfg.timeout.tv_sec > 0) {
	 	uint32_t delta = current_time -  send_mess.sent_time;
	 	if( delta >  gp_cfg.gp_timeout) {
			if( send_mess.activ > 0 ) {
				send_mess.activ=0;
				devices[send_mess.dst].timeout_count++;
//				fprintf(stderr, "inc %d dev=%d\n", devices[send_mess.dev].timeout_count, send_mess.dev);
//				if( gp_cfg.polling == 1 ) {
				if( send_mess.dev == 7 ) {
					gp_cfg.last_target=0;
					fprintf(stderr, "Timeout for read for %d (cmd=%d, delta=%u)\n", 
						send_mess.dev, gp_cfg.last_target, current_time - gp_cfg.last_read);
					syslog(LOG_ERR, "Timeout for read for %d (cmd=%d, delta=%u)",   
						send_mess.dev, gp_cfg.last_target, current_time - gp_cfg.last_read);
				}
			}
		}	 	
//		if( current_time >= expect_time + gp_cfg.gp_timeout) {
	 	if( current_time - gp_cfg.last_read >  gp_cfg.gp_timeout*2) {
			if( gp_put_cmd() > 0 ||  gp_send_idle() > 0) return;
//	 		fprintf(stderr, "Runed gp_put_cmgp_cfgd!\n");
	 	}
		evutil_timerclear(&gp_cfg.timeout);
		gp_cfg.timeout.tv_usec = gp_cfg.gp_timeout * 1000;
		if ( event_add(&evport, &gp_cfg.timeout) < 0) syslog(LOG_ERR, "event_add.send_mess.ev_timeout setup: %m");
		return;
	}

// HACK Read (EV_READ)
	evutil_timerclear(&gp_cfg.timeout);
	gp_cfg.timeout.tv_usec = gp_cfg.gp_timeout * 1000;
 	if ( event_add(&evport, &gp_cfg.timeout) < 0) syslog(LOG_ERR, "event_add.send_mess.ev_timeout setup: %m");

// 	fprintf(stderr, "Set last read %lu \n",current_time);
	gp_cfg.last_read=current_time;
	gp_cfg.last_dev=send_mess.dst;
	send_mess.activ=0;
	if( gp_cfg.polling == 1 && devices[send_mess.dev].activ==0) {
		syslog(LOG_ERR,"Found dev: '%d'\n",send_mess.dev);
	}
	devices[send_mess.dst].activ=1; 
	devices[send_mess.dst].timeout_count=0;
// HACK	
	mydev=send_mess.dst;
// First chunk of data. 
	if( receiv_buf_p == 0 ) {
		receiv_buf_p = receiv_mess.buf;
		receiv_mess.len=0;
	}
	nread = read(fd, receiv_buf_p, GP_PORT_READ_BUF_LENGTH - 1 - receiv_mess.len);
	
	
	if( debug >= 5 || (debug >= 4 && gp_cfg.polling == 0) ||  receiv_mess.src != 7) {
		print_buff("port received: ", receiv_buf_p, nread);
	}
// 	dprint(DL5, "read %d bytes, ev=%x\n", nread, event);
	
	if (nread < 0) { /* EOF */
		syslog(LOG_CRIT, "read: %m");
		gp_close();
		// daemon_exit(1); // never exit on error!
	} else if (nread == 0) {
//		syslog(LOG_ERR, "port socket unexpectedly closed");
	 	fprintf(stderr, "port unexpectedly closed\n");
		gp_reconnect(1);
//		gp_close();
		return;
	} else if (nread > 0) {
		// right trim buffer
		receiv_buf_p[nread] = '\0';
		if( receiv_mess.fl_begin !=  GP_INIT) {
			syslog(LOG_ERR, "Begin flag not found");
			gp_reconnect(1);
//			gp_close();
			return;
		}		
		uint8_t* p=memchr(receiv_buf_p, GP_END,nread);
		if ( p != 0 ) {
			receiv_buf_p=0;
//			*p='\0'; 
			int l = p - &receiv_mess.id_ctrl; // buff. length  for crc		
//	fprintf(stderr, "l = %d bytes\n", l);
//			l = memcpy_unesc(receiv_mess.cmd_buff,receiv_mess.cmd_buff,l);
			l = memcpy_unesc(&receiv_mess.id_ctrl,&receiv_mess.id_ctrl,l);
//	fprintf(stderr, "l = %d bytes\n", l);
			if ( crc8_xor(&receiv_mess.id_ctrl,l) > 0 ) {
				int crc=crc8_xor(&receiv_mess.id_ctrl,l-1);
				fprintf(stderr, "crc8 error for %d (cmd=%d, crc=x0%02x) \n", send_mess.dst, gp_cfg.last_target, crc);
				syslog(LOG_ERR, "crc8 error for %d (cmd=%d, crc=x0%02x) \n", send_mess.dst, gp_cfg.last_target, crc);
				if( debug < 5 ) 
					print_buff("port received (crc error): ", receiv_mess.buf, nread+receiv_mess.len);
//				return;
			}
			if( receiv_mess.cmd_n == GP_REPLY ) {
//				syslog(LOG_ERR, "Got ask(0x%02X) for %d (cmd=%d)\n",receiv_mess.replay, send_mess.dst, gp_cfg.last_target);
				if( receiv_mess.replay == GP_REPLY_ACK ) {
					fprintf(stderr, "Got ACK for (0x%02X) \n", receiv_mess.was_cmd_n);
				} else if ( receiv_mess.replay == GP_REPLY_NACK ) {
					fprintf(stderr, "Got NACK for (0x%02X) \n", receiv_mess.was_cmd_n);
				} else {
					fprintf(stderr, "Got  EEPROM/RTC error (%d)\n",receiv_mess.replay );
				}
				
			} else {
				receiv_mess.len=l-(3+1);
				if( process_port_input() > 0 ) return;
			}
		} else {
			receiv_mess.len +=nread;
			receiv_buf_p+=nread;
			return;
		}
//			dprint(DL2, "port recvd: '%.*s'\n", nread, buf);
/*			if(debug >= DL2) {
				char buf2[PORT_READ_BUF_LENGTH];
				char* p=strncpy(buf2,buf,PORT_READ_BUF_LENGTH);
				while( (p=strchr(p,'\r')) > 0 ) *p='$';
				p=buf2;
				while( (p=strchr(buf2,'\n')) >0 ) *p='&';
				dprint(DL2, "port recvd: '%s'\n", buf2);
			}*/
	}
//	fprintf(stderr, "Sending new cmd\n");
	gp_put_cmd();
// 	if ( gp_put_cmd() == 0 && event_add(&evport, &gp_cfg.timeout) < 0) 
// 		syslog(LOG_ERR, "event_add.send_mess.ev_timeout setup: %m");

// exit(0);	f
// fprintf(stderr, "gp_receiv ok \n");
}

void gp_close () {
	if (gp_cfg.fd >= 0) {
		syslog(LOG_ERR, "closing controller port %m");
//		if ( event_del(&evport) < 0) syslog(LOG_ERR, "event_del (port): %m");
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
				syslog(LOG_CRIT, "open port (errcode %d): %m", errno);
//				if (portIsOpen == 1) send_switch(GATEKEEPER_NUMERIC_ID | NAGIOS_NUMERIC_ID, ALERT_TTY_USB);
				portIsOpen=0;
			} else {
				fprintf(stderr, "use port %s (%d)\n", gp_cfg.port_path, gp_cfg.fd);
//				if (adapter_init() < 0) syslog(LOG_CRIT, "cannot initialize adapter");
//				event_set(&evport, gp_cfg.fd, EV_READ|EV_PERSIST, gp_receiv, (void*)&evport);
//				event_set(&evport, gp_cfg.fd, EV_READ|EV_PERSIST|EV_TIMEOUT, gp_receiv, (void*)&evport);
				event_set(&evport, gp_cfg.fd, EV_READ|EV_TIMEOUT, gp_receiv, (void*)&evport);
				if ( event_add(&evport, &gp_cfg.timeout) < 0) syslog(LOG_ERR, "evport,event_add setup: %m");
				portIsOpen=1;
			}
		}
	}
	if( gp_cfg.fd < 0 ) 
		return 0;
	else
		return 1;
}

int gp_init () {
	int i;
	for(i=1;i<=255;i++) {
//		devices[i].bound_tocken=0x2000;
		devices[i].bound_tocken=GP_SPART_TICKET_BOUND;
	}
 	event_set(&gp_cfg.ev_scan, -1 , EV_TIMEOUT|EV_PERSIST, gp_all_dev_ev, (void*)&gp_cfg.ev_scan);
 	gp_all_dev_ev(0,0,0);
	// connect to controller port and switch socket immediately
	gp_reconnect(1);
	return 1;
}