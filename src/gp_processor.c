/*
 *
 * Adapter for controller of gate
 * 
 * Designed by Evgeny Byrganov <eu dot safeschool at gmail dot com> for safeschool.ru, 2012
 *  
 * $Id: gp_processor.c 2856 2012-11-23 13:29:04Z eu $
 *
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
//#include <syslog.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#include "libgate.h"

int cb_push_new_cid_default (proc_add_cid_t rec) {
	zprintf(4, "dev: %u, subdev: %u, ev: 0x%02x, cid: 0x%016llX\n", 
 			rec.dev, rec.subdev, rec.ev, rec.token.cid );
	
	zprintf(8,"Writing  cid in all devs\n");
	
	rec.token.schedule_mask=0xFF;
	rec.token.attr=0;
	return cmd_add_cid_to_all(rec);
}
int cb_push_found_cid_default (proc_add_cid_t rec) {
	zprintf(4, "dev: %u, subdev: %u, ev: 0x02x, cid: 0x%016llX\n", 
 			rec.dev, rec.subdev, rec.ev, rec.token.cid );
	return 1;
}

char* if_types_str[]={ "touch memory", "Wiegand", "ABA2", "Unknow" };

int cb_dev_info_default(proc_info_t rec) {
	int if_type =(rec.if_type >=0 &&  rec.if_type  < 3 )? rec.if_type:3;
	zprintf(4, "dev: %d, ver: 0x%02x, release=%d, model: 0x%x, if_type: %s (%u)\n", 
 			rec.dev, rec.ver, rec.release, rec.model, if_types_str[if_type], if_type);
	return 1;
}

int cb_dev_ev_default(proc_event_t rec) {
	zprintf(6, "ev (%d): code=%x, addr=0x%04X, cid: 0x%016llX (dup: %u)  %s",  
		rec.dev, rec.ev, rec.addr,  rec.cid, rec.dup, asctime (rec.ct));
	return 1;
}

int cb_dev_sched_default() {
// 	zprintf(6, "ev (%d): code=%x, addr=0x%04X, cid: 0x%016llX (dup: %u)  %s",  
// 		rec.dev, rec.ev, rec.addr,  rec.cid, rec.dup, asctime (rec.ct));
	return 1;
}

int (*cb_push_new_cid_handler)(proc_add_cid_t rec) = &cb_push_new_cid_default;
int (*cb_push_found_cid_handler)(proc_add_cid_t rec) = &cb_push_found_cid_default;
int (*cb_dev_info_handler)(proc_info_t rec) = &cb_dev_info_default;
int (*cb_dev_ev_handler)(proc_event_t rec) = &cb_dev_ev_default;
//int (*cb_dev_sched_default)() = &cb_dev_sched_default;

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
			
			zprintf(6,"dev=%d, ev=%02x, cid: 0x%016llX\n", receiv_mess.dev,  t->ev, devices[receiv_mess.dev].last_cid);
// новая карта!!!		
			if( (t->ev & ~1 ) == 0x02 ) {
				proc_add_cid_t tt = { 
					.dev = receiv_mess.dev,
					.subdev = t->ev & 1,
//					.cid = devices[receiv_mess.dev].last_cid,
					.ev = t->ev
				};

// TODO	 - check cid in  db, and if not found - added
				db_token_rec_t* token=db_get_token_by_c(devices[receiv_mess.dev].last_cid);
				if( token != NULL  ) {
					ad_turn_on(AD_Q_SECOND, tt.dev, tt.subdev);
					memcpy(&tt.token, token, sizeof(db_token_rec_t));
					if( cmd_add_cid(tt) == 0) {
						tt.token.addr=0;
						cmd_add_cid_to_all(tt);
					}
				} else {
					tt.token.cid = devices[receiv_mess.dev].last_cid;
					cb_push_new_cid_handler(tt);
				}
// TODO 				
			} else
			if( (t->ev & ~1 ) == 0x04 ) {
				proc_add_cid_t tt = { 
					.dev = receiv_mess.dev,
					.subdev = t->ev & 1,
//					.cid = devices[receiv_mess.dev].last_cid,
					.ev = t->ev
				};
				tt.token.cid = devices[receiv_mess.dev].last_cid;
				cb_push_found_cid_handler(tt);
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
		
		if( bound_down > GP_MAX_LOG_BOUND ||  bound_up > GP_MAX_LOG_BOUND) {
			zprintf(2, "Bad addr_buff (%d): up=0x%04X down=0x%04X\n", 
				receiv_mess.dev, bound_up, bound_down);
			return 0;
		}
		
		int l = bound_up - bound_down;
		if( l < 0 ) {
			l = GP_MAX_LOG_BOUND - bound_down;
		}
		size_t l2 = gp_cfg.ev_block_size*sizeof(gp_event_t);
		l=(l<l2)? l:l2;
		
		if( devices[receiv_mess.dev].bound_down != bound_down ) {
			zprintf(6, "addr_buff (%d): up=0x%04X down=0x%04X, len=%d. count=%d  \n", 
				receiv_mess.dev, bound_up, bound_down,l, l / sizeof(gp_event_t));
//			syslog(LOG_ERR, "addr_buff: down=0x%04X up=0x%04X, len=%d \n", bound_up, bound_down,l );
		}
		devices[receiv_mess.dev].bound_down=bound_down;
		devices[receiv_mess.dev].bound_up=bound_up;
		
		if(l != 0) return ad_get_buff_data(AD_Q_SHORT,receiv_mess.src,bound_down,l);
	} else if ( send_mess.target == AD_TARGET_GET_BUFF_DATA) {
		gp_event_t* t=(gp_event_t*)&receiv_mess.cmd_buff;
		gp_event_t* bound=(gp_event_t*)devices[receiv_mess.dev].bound_down; // we make virtual pointer (addr in device) on gp_event_t 
		int n=receiv_mess.len/sizeof(gp_event_t);
		int i, c=0;
		for(i=0;i<n;i++,t++,bound++) {
//			if(  memcmp( &devices[receiv_mess.dev].last_ev_rec, t, sizeof(gp_event_t))  ) {
			if(  memcmp( &devices[receiv_mess.dev].last_ev_rec, t, sizeof(uint8_t)+sizeof(uint16_t))  ) {
// TODO здесь  2 сек. задержка
				memcpy(&devices[receiv_mess.dev].last_ev_rec, t, sizeof(gp_event_t));
/*				db_token_rec_t* token = db_get_token_by_a(htobe16(t->addr));
				uint64_t cid = (token)? token->cid:0;*/
				uint64_t cid = db_get_cid_by_a((uint32_t)htobe16(t->addr));
				proc_event_t rec = {
					.dev = receiv_mess.dev,
					.subdev = t->ev_code & 1,
					.ev = t->ev_code,
					.addr = htobe16(t->addr),
					.cid = cid,
					.ts = get_ev_time(t),
					.ct = get_ev_date(t),
					.dup = c
				};
				
				cb_dev_ev_handler(rec);
//				syslog(LOG_ERR, "ev (%d): code=%x, addr=0x%04X, 2012-%02x-%02x %02x:%02x:%02x (%lu),  cid: 0x%016llX\n",  receiv_mess.dev,
//					t->ev_code, htobe16(t->addr), t->mon, t->day, t->hour , t->min, t->sec,	get_ev_time(t), cid);
//					fprintf(stderr, "ev time: %u\n", get_ev_time(t));
//					fprintf(stderr, "ev time: %u, date: %s\n", get_ev_time(t), asctime(get_ev_date(t)));
			} else {
				c++;
			}
		} 
//			fprintf(stderr, "ev: lastbound=0x%04X \n", bound);
		if (bound >= GP_MAX_LOG_BOUND) bound=0;
		return ad_set_buff_addr_down(AD_Q_SHORT,receiv_mess.src,(uint16_t)bound);	
	} else if ( send_mess.target == AD_TARGET_SET_BUFF_ADDR_DOWN) {
		zprintf(6, "Has written new buff down address  \n", receiv_mess.was_cmd_n);
		return 1;
	}
	return 0;
}

int process_port_input(int reply) {
	if( debug >= 9 || (debug >= 8 && gp_cfg.polling == 0)  ) {
		zprintf(8, "cmd_buff: "); 
		int i=0; for(i=0; i < receiv_mess.len ;i++) {
			zprintf(8, "0x%02x, ", receiv_mess.cmd_buff[i]);
		}
		zprintf(8, "cmd: %02x, len=%u\n", receiv_mess.cmd_n,receiv_mess.len);
 	}
 
/*	if( cb_get_poll_result(1) ) return 1;
	if( cb_log_read(1) ) return 1;*/
//	fprintf(stderr, "Call handler 0x%lx for %d\n", (uint32_t)send_mess.ev_handler, send_mess.target );
	
 	if( send_mess.ev_handler != 0) {
		int r = (*send_mess.ev_handler)(reply);
		return r;
 	}
/* 	
cmd_buff: 0xc2, 0x10, 0xe1, 0x01, cmd: 09, len=4
dev: 1, ver: 0x02, release=1,  model: 0xc, if_type: 0
sent into socket: '01:84:EV:01ri: ver: 0x02, release=1,  model: 0xc, if_type: 0

cmd_buff: 0x41, 0x10, 0xed, 0x0d, cmd: 09, len=4
dev: 3, ver: 0x01, release=13, model: 0x4, if_type: 1
sent into socket: '01:84:EV:03ri: ver: 0x01, release=13, model: 0x4, if_type: 1'
*/ 	
 	if( receiv_mess.cmd_n == 0x09 ) {
 		gp_info_t* t=(gp_info_t*)receiv_mess.cmd_buff;
// 		memcpy(&t,receiv_mess.cmd_buff,( (receiv_mess.len <= sizeof t )? receiv_mess.len : 4));
 		int if_type=(t->if_type_n)? t->if_type2:t->if_type1;
 		devices[receiv_mess.dev].cid_revert = (if_type > 0)? 1:0;
 		proc_info_t rec = {
 			.dev = receiv_mess.dev,
 			.ver = t->ver,
 			.model = t->model,
 			.release = t->release,
 			.if_type = if_type
 		};
 		
 		if( rec.model >= 0x0C) {
 			devices[receiv_mess.dev].max_bound_token=0xFFF0;
 			devices[receiv_mess.dev].max_bound_log=GP_MAX_LOG_BOUND;
 		}
 		if( cb_dev_info_handler > 0) cb_dev_info_handler(rec); 
// 		else cb_dev_info_default(rec);
 		
 		
 	} else if ( receiv_mess.cmd_n == 0x0A ) {
 		gp_port_status_t t;
 		memcpy(&t,receiv_mess.cmd_buff,( (receiv_mess.len <= sizeof t )? receiv_mess.len : 4));
 	} else if ( receiv_mess.cmd_n == 0x04 ) {
  		if ( send_mess.target == AD_TARGET_GET_TOKEN_BOUND) {
			gp_addr1_t* t = (gp_addr1_t*)receiv_mess.cmd_buff;
 			uint16_t bound=be16toh(t->bound);
 //			if(  devices[receiv_mess.dev].bound_token != bound) {
 				devices[receiv_mess.dev].bound_token=bound;
				zprintf(7, "dev=%d: token_buff_%d: bound=0x%04X\n", receiv_mess.dev, send_mess.bank, bound);
//				syslog(LOG_ERR, "dev=%d: token_buff_%d: bound=0x%04X\n", receiv_mess.dev, send_mess.bank, bound);
//			}
  		
		} else 	if ( send_mess.target == AD_TARGET_GET_BUFF_ADDR) {
			gp_addr2_t* t = (gp_addr2_t*)receiv_mess.cmd_buff;
			uint16_t bound_down=be16toh(t->bound_down);		
			uint16_t bound_up=be16toh(t->bound_up);
			zprintf(6, "addr_buff (%d): up=0x%04X down=0x%04X\n", 
				receiv_mess.dev, bound_up, bound_down);
//			devices[receiv_mess.dev].bound_down=bound_down;
//			devices[receiv_mess.dev].bound_up=bound_up;
		
  		} else if ( send_mess.target == AD_TARGET_GET_TIMES) {
  			gp_times_t* t=(gp_times_t*)&receiv_mess.cmd_buff;
			zprintf(6, "%d: open_door=%u, ctrl_close=%u, ctrl_open=%u, timeout_confirm=%u\n", 
				send_mess.bank, t->open_door, t->ctrl_close, t->ctrl_open, t->timeout_confirm );
  		} else if ( send_mess.target == AD_TARGET_GET_DATE) {
  			gp_date_rtc_t* t=(gp_date_rtc_t*)&receiv_mess.cmd_buff;
			zprintf(4, "RTC date (dev=%d): 20%02x-%02x-%02x %02x:%02x:%02x  (%s)\n", receiv_mess.dev,
				t->year, t->mon, t->day, t->hour , t->min, t->sec, (t->stop)? "stopped":"working" );
//			fprintf(stderr, "RTC date: %s\n", asctime(get_rtc_date(t)));
/*			syslog(LOG_ERR, "RTC date (dev=%d): 20%02x-%02x-%02x %02x:%02x:%02x\n", receiv_mess.dev,
				t->year, t->mon, t->day, t->hour , t->min, t->sec);*/
  		} else if ( send_mess.target == AD_TARGET_GET_SCHED) {
  			gp_time_zone_t* t=(gp_time_zone_t*)&receiv_mess.cmd_buff;
  			int i=0;
			for(i=0;i<7;i++,t++) {
				zprintf(4, "Sched (dev=%d.%d.%d): %02x %02x:%02x %02x:%02x\n", receiv_mess.dev, send_mess.bank, i,
					t->wmask, t->begin_hour, t->begin_min, t->end_hour , t->end_min );
  			}
  		} else if ( send_mess.target == AD_TARGET_GET_TOKEN) {
  			gp_token_rec_t* t=(gp_token_rec_t*)&receiv_mess.cmd_buff;
			uint64_t a=bin2int(t->cid,6,devices[receiv_mess.dev].cid_revert);
  			int i=0;
			for(i=0;i<1;i++,t++) {
				zprintf(4, "cid: (dev=%d.%d.%d): 0x%016llX,  %02x, %02x\n", receiv_mess.dev, send_mess.bank, i,
					a, t->attr, t->schedule_mask);
			}
  		}
	}
 	return 0;
}

