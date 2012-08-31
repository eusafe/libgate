/*
 *
 * Adapter for controller of gate
 * 
 * Designed by Evgeny Byrganov <eu dot safeschool at gmail dot com> for safeschool.ru, 2012
 *  
 * $Id$
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

#include "libgate.h"



int process_port_input() {
	if( debug >= 5 || (debug >= 4 && gp_cfg.polling == 0)  ) {
		fprintf(stderr, "cmd_buff: "); 
		int i=0; for(i=0; i < receiv_mess.len ;i++) {
			fprintf(stderr, "0x%02x, ", receiv_mess.cmd_buff[i]);
		}
		fprintf(stderr, "cmd: %02x, len=%u\n", receiv_mess.cmd_n,receiv_mess.len);
 	}
 
 	if( receiv_mess.cmd_n == 0x09 ) {
 		gp_info_t t;
 		memcpy(&t,receiv_mess.cmd_buff,( (receiv_mess.len <= sizeof t )? receiv_mess.len : 4));
 		int if_type=(t.if_type_n)? t.if_type2:t.if_type1;
 		fprintf(stderr, "ver: 0x%02x, model: 0x%u, if_type: %u, release=%d\n", 
 			t.ver, t.model,if_type , t.release );
 		devices[gp_cfg.last_dev].cid_revert = (if_type > 0)? 1:0;
 		
 	} else if ( receiv_mess.cmd_n == 0x0A ) {
 		gp_port_status_t t;
 		memcpy(&t,receiv_mess.cmd_buff,( (receiv_mess.len <= sizeof t )? receiv_mess.len : 4));
 	} else if ( receiv_mess.cmd_n == 0x06 ) {
 		gp_reg_cid_t* t=(gp_reg_cid_t*)receiv_mess.cmd_buff;
 		if( devices[gp_cfg.last_dev].new_cid_flag == 1 || devices[gp_cfg.last_dev].last_ev != t->ev ) {
			uint8_t cid[20];
// 			uint8_t t[20]= {'0','0','0','0'};
// 			int l=bin2hex(&t,&receiv_mess.cmd_buff,( (receiv_mess.len <= 6)? receiv_mess.len : 6), devices[gp_cfg.last_dev].cid_revert);

//	 		bin2hex(cid,t->cid,6,devices[gp_cfg.last_dev].cid_revert);
//			fprintf(stderr, "cid: 0x%12s, addr: %d, ev: %02x, src: %d\n", cid,t->addr,t->ev, gp_cfg.last_dev);
			long2hex(cid,devices[gp_cfg.last_dev].last_cid);
			fprintf(stderr,"cid: 0x%016llX,  0x%12s dev=%d, ev: %02x\n", devices[gp_cfg.last_dev].last_cid,
				cid, gp_cfg.last_dev, t->ev);
//			syslog(LOG_ERR,"cid: 0x%12s dev=%d, ev: %02x", cid, gp_cfg.last_dev,t->ev);
		
			if( (t->ev & ~1 ) == 0x02 ) {
				gp_token_rec_t t2;
				memset(&t2,0,sizeof(gp_token_rec_t));
				int2bin(t2.cid, devices[gp_cfg.last_dev].last_cid,6,devices[gp_cfg.last_dev].cid_revert);
//				memcpy(&t2,(void*)devices[gp_cfg.last_dev].last_cid,6);
				t2.attr=0; t2.time_zone_mask=0xFF;
				ad_set_token(AD_Q_SECOND, gp_cfg.last_dev, devices[gp_cfg.last_dev].bound_tocken, &t2,1);
				fprintf(stderr,"Writing  cid: 0x%012X 12s dev=%d, .bound=0x%02X\n", 
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
			uint64_t a=bin2int(receiv_mess.cmd_buff,receiv_mess.len,devices[gp_cfg.last_dev].cid_revert);
			if( devices[gp_cfg.last_dev].last_cid != a ) {
				devices[gp_cfg.last_dev].last_cid=a;
				devices[gp_cfg.last_dev].new_cid_flag=1;
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
  			gp_event_t* t=(gp_event_t*)&receiv_mess.cmd_buff;
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

