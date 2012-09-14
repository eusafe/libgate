/*
 *
 * Adapter for controller of gate
 * 
 * Designed by Evgeny Byrganov <eu dot safeschool at gmail dot com> for safeschool.ru, 2012
 *  
 * $Id: gp_processor.c 2731 2012-09-14 14:55:56Z eu $
 *
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#include "libgate.h"


int cs_push_new_cid (proc_add_cid_t rec) {
	cmd_add_cid(rec);
	return 1;
}

int (*cs_push_new_cid_handler)(proc_add_cid_t rec) = &cs_push_new_cid;

int cb_get_poll_result(int reply) {
//	fprintf(stderr, "Run cb_get_poll_result for %d\n", send_mess.target );
	
	if( send_mess.target == AD_TARGET_GET_CID && receiv_mess.cmd_n == 0x04){
		uint64_t a=bin2int(receiv_mess.cmd_buff,receiv_mess.len,devices[receiv_mess.dev].cid_revert);
		if( devices[receiv_mess.dev].last_cid != a ) {
			devices[receiv_mess.dev].last_cid=a;
			devices[receiv_mess.dev].new_cid_flag=1;
		}
		return ad_get_regs(AD_Q_SHORT, receiv_mess.src);
 	} else if (send_mess.target == AD_TARGET_GET_REGS && receiv_mess.cmd_n == 0x06 ) {
 		gp_reg_cid_t* t=(gp_reg_cid_t*)receiv_mess.cmd_buff;
 		if( devices[receiv_mess.dev].new_cid_flag == 1 || devices[receiv_mess.dev].last_ev != t->ev ) {
// 			uint8_t t[20]= {'0','0','0','0'};
// 			int l=bin2hex(&t,&receiv_mess.cmd_buff,( (receiv_mess.len <= 6)? receiv_mess.len : 6), devices[receiv_mess.dev].cid_revert);

//	 		bin2hex(cid,t->cid,6,devices[receiv_mess.dev].cid_revert);
//			fprintf(stderr, "cid: 0x%12s, addr: %d, ev: %02x, src: %d\n", cid,t->addr,t->ev, receiv_mess.dev);

//			uint8_t cid[20];
//			long2hex(cid,devices[receiv_mess.dev].last_cid);
			
			fprintf(stderr,"dev=%d, ev=%02x, cid: 0x%016llX\n", receiv_mess.dev,  t->ev, devices[receiv_mess.dev].last_cid);
//			syslog(LOG_ERR,"cid: 0x%12s dev=%d, ev: %02x", cid, receiv_mess.dev,t->ev);
		
// новая карта!!!		
			if( (t->ev & ~1 ) == 0x02 ) {
				proc_add_cid_t tt = { 
					.dev = receiv_mess.dev,
					.subdev = t->ev & 1,
					.cid = devices[receiv_mess.dev].last_cid,
					.ev = t->ev
				};
// TODO	 - check cid in  db, and if not found - added
//				cmd_add_cid(tt);
				cs_push_new_cid_handler(tt);
// TODO 				
/*				gp_token_rec_t t2;
				memset(&t2,0,sizeof(gp_token_rec_t));
				int2bin(t2.cid, devices[receiv_mess.dev].last_cid,6,devices[receiv_mess.dev].cid_revert);
//				memcpy(&t2,(void*)devices[receiv_mess.dev].last_cid,6);
				t2.attr=0; t2.time_zone_mask=0xFF;
				ad_set_token(AD_Q_SECOND, receiv_mess.dev, devices[receiv_mess.dev].bound_token, &t2,1);
				fprintf(stderr,"Writing  cid: 0x%012llX, dev=%d, bound=0x%04X\n", 
					devices[receiv_mess.dev].last_cid, receiv_mess.dev, devices[receiv_mess.dev].bound_token);
//				devices[receiv_mess.dev].bound_token+=8;
				ad_set_token_bound(AD_Q_SECOND, receiv_mess.dev, devices[receiv_mess.dev].bound_token);
				ad_turn_on(AD_Q_SECOND, receiv_mess.dev, t->ev & 1);*/
			}
		}
		devices[receiv_mess.dev].last_ev=t->ev;
		devices[receiv_mess.dev].new_cid_flag = 0;
		return 1;
	}
	return 0;
}

int cb_log_read(int reply) {
//	fprintf(stderr, "Run cb_log_read for %d\n", send_mess.target );
	if ( send_mess.target == AD_TARGET_GET_BUFF_ADDR) {
		gp_addr2_t* t = (gp_addr2_t*)receiv_mess.cmd_buff;
		uint16_t bound_down=be16toh(t->bound_down);		
		uint16_t bound_up=be16toh(t->bound_up);
		size_t l = bound_up - bound_down;
		size_t l2 = gp_cfg.ev_block_size*sizeof(gp_event_t);
		
		if( devices[receiv_mess.dev].bound_down != bound_down ) {
			fprintf(stderr, "addr_buff (%d): up=0x%04X down=0x%04X, len=%d. sizeof=%d  \n", 
				receiv_mess.dev, bound_up, bound_down,l, sizeof(gp_event_t));
//			syslog(LOG_ERR, "addr_buff: down=0x%04X up=0x%04X, len=%d \n", bound_up, bound_down,l );
		}
		devices[receiv_mess.dev].bound_down=bound_down;
		devices[receiv_mess.dev].bound_up=bound_up;
		
		l=(l>l2)? l2:l;
		if(l != 0) return ad_get_buff_data(AD_Q_SHORT,receiv_mess.src,bound_down,l);
	} else if ( send_mess.target == AD_TARGET_GET_BUFF_DATA) {
		gp_event_t* t=(gp_event_t*)&receiv_mess.cmd_buff;
		gp_event_t* bound=(gp_event_t*)devices[receiv_mess.dev].bound_down; // we make virtual pointer (addr in device) on gp_event_t 
		int n=receiv_mess.len/sizeof(gp_event_t);
		int i;
		for(i=0;i<n;i++,t++,bound++) {
			if(  memcmp( &devices[receiv_mess.dev].last_ev_rec, t, sizeof(gp_event_t))  ) {
				memcpy(&devices[receiv_mess.dev].last_ev_rec, t, sizeof(gp_event_t));
				fprintf(stderr, "ev (%d): code=%x, addr=0x%04X, 2012-%02x-%02x %02x:%02x:%02x (%lu), dev=%d \n",  receiv_mess.dev,
					t->ev_code, htobe16(t->addr), t->mon, t->day, t->hour , t->min, t->sec, get_ev_time(t), receiv_mess.dev);
				syslog(LOG_ERR, "ev (%d): code=%x, addr=0x%04X, 2012-%02x-%02x %02x:%02x:%02x (%lu), dev=%d \n",  receiv_mess.dev,
					t->ev_code, htobe16(t->addr), t->mon, t->day, t->hour , t->min, t->sec,	get_ev_time(t),receiv_mess.dev);
//					fprintf(stderr, "ev time: %u\n", get_ev_time(t));
//					fprintf(stderr, "ev time: %u, date: %s\n", get_ev_time(t), asctime(get_ev_date(t)));
			}
		} 
//			fprintf(stderr, "ev: lastbound=0x%04X \n", bound);
		return ad_set_buff_addr_down(AD_Q_SHORT,receiv_mess.src,(uint16_t)bound);	
	} else if ( send_mess.target == AD_TARGET_SET_BUFF_ADDR_DOWN) {
		fprintf(stderr, "Has written new buff down address  \n", receiv_mess.was_cmd_n);
		return 1;
	}
	return 0;
}

int process_port_input(int reply) {
	if( debug >= 5 || (debug >= 4 && gp_cfg.polling == 0)  ) {
		fprintf(stderr, "cmd_buff: "); 
		int i=0; for(i=0; i < receiv_mess.len ;i++) {
			fprintf(stderr, "0x%02x, ", receiv_mess.cmd_buff[i]);
		}
		fprintf(stderr, "cmd: %02x, len=%u\n", receiv_mess.cmd_n,receiv_mess.len);
 	}
 
/*	if( cb_get_poll_result(1) ) return 1;
	if( cb_log_read(1) ) return 1;*/
//	fprintf(stderr, "Call handler 0x%lx for %d\n", (uint32_t)send_mess.ev_handler, send_mess.target );
	
 	if( send_mess.ev_handler != 0) {
		int r = (*send_mess.ev_handler)(reply);
		return r;
 	}
 	
 	if( receiv_mess.cmd_n == 0x09 ) {
 		gp_info_t t;
 		memcpy(&t,receiv_mess.cmd_buff,( (receiv_mess.len <= sizeof t )? receiv_mess.len : 4));
 		int if_type=(t.if_type_n)? t.if_type2:t.if_type1;
 		fprintf(stderr, "dev: %d, ver: 0x%02x, model: 0x%u, if_type: %u, release=%d\n", 
 			receiv_mess.dev, t.ver, t.model,if_type , t.release );
 		devices[receiv_mess.dev].cid_revert = (if_type > 0)? 1:0;
 		
 	} else if ( receiv_mess.cmd_n == 0x0A ) {
 		gp_port_status_t t;
 		memcpy(&t,receiv_mess.cmd_buff,( (receiv_mess.len <= sizeof t )? receiv_mess.len : 4));
 	} else if ( receiv_mess.cmd_n == 0x04 ) {
/*  		uint64_t cid=0;
 		memcpy(&cid,receiv_mess.cmd_buff,( (receiv_mess.len <= sizeof  cid)? receiv_mess.len : sizeof  cid));
  		fprintf(stderr, "cid: 0x%012llX\n",  htobe64(cid << 16));*/\
//   		if( send_mess.target == AD_TARGET_GET_CID ){
// 			uint64_t a=bin2int(receiv_mess.cmd_buff,receiv_mess.len,devices[receiv_mess.dev].cid_revert);
// 			if( devices[receiv_mess.dev].last_cid != a ) {
// 				devices[receiv_mess.dev].last_cid=a;
// 				devices[receiv_mess.dev].new_cid_flag=1;
// 			}
// 			return ad_get_regs(AD_Q_SHORT, receiv_mess.src);
//   		} else 
  		if ( send_mess.target == AD_TARGET_GET_TOKEN_BOUND) {
			gp_addr1_t* t = (gp_addr1_t*)receiv_mess.cmd_buff;
 			uint16_t bound=be16toh(t->bound);
 //			if(  devices[receiv_mess.dev].bound_token != bound) {
 				devices[receiv_mess.dev].bound_token=bound;
				fprintf(stderr, "dev=%d: token_buff_%d: bound=0x%04X\n", receiv_mess.dev, send_mess.bank, bound);
//				syslog(LOG_ERR, "dev=%d: token_buff_%d: bound=0x%04X\n", receiv_mess.dev, send_mess.bank, bound);
//			}
  		} else if ( send_mess.target == AD_TARGET_GET_TIMES) {
  			gp_times_t* t=(gp_times_t*)&receiv_mess.cmd_buff;
/*			fprintf(stderr, "%d: open_door=%u, ctrl_close=%u, ctrl_open=%u, timeout_confirm=%u\n", 
				send_mess.bank, t->open_door, t->ctrl_close, t->ctrl_open, t->timeout_confirm );
			syslog(LOG_ERR, "%d: open_door=%u, ctrl_close=%u, ctrl_open=%u, timeout_confirm=%u\n", 
				send_mess.bank, t->open_door, t->ctrl_close, t->ctrl_open, t->timeout_confirm );*/
  		} else if ( send_mess.target == AD_TARGET_GET_DATE) {
  			gp_date_rtc_t* t=(gp_date_rtc_t*)&receiv_mess.cmd_buff;
			fprintf(stderr, "RTC date (dev=%d): 20%02x-%02x-%02x %02x:%02x:%02x  (%s)\n", receiv_mess.dev,
				t->year, t->mon, t->day, t->hour , t->min, t->sec, (t->stop)? "stopped":"working" );
//			fprintf(stderr, "RTC date: %s\n", asctime(get_rtc_date(t)));
			syslog(LOG_ERR, "RTC date (dev=%d): 20%02x-%02x-%02x %02x:%02x:%02x\n", receiv_mess.dev,
				t->year, t->mon, t->day, t->hour , t->min, t->sec);
  			
  		}
	}
 	return 0;
}

