/*
 *
 * Adapter for controller of gate
 * 
 * Designed by Evgeny Byrganov <eu dot safeschool at gmail dot com> for safeschool.ru, 2012
 *  
 * $Id: ad_target.c 2772 2012-11-01 11:46:08Z eu $
 *
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/queue.h>

#include "libgate.h"

int ad_get_info(int q, int gp_dst) {
	cmd_send_t z = {
		.queue = q,
		.dst = gp_dst,
		.ev_handler = 0,
		.target_n = AD_TARGET_INFO,
		.cmd_n = 0x09,
		.set_timeout = 100,              
		.polling = 0,
		.data_len = 0,
	};
	if( q == AD_Q_SHORT ) 
		return gp_send(&z);
	else
		return gp_in_cmd(&z);
}

int ad_soft_reset(int q, int gp_dst) {
	cmd_send_t z = {
		.queue = q,
		.dst = gp_dst,
		.ev_handler = 0,
		.target_n = AD_TARGET_SOFT_RESET,
		.cmd_n = 0x0B,
		.set_timeout = 999,
		.polling = 0,
		.data_len = 0,
	};
	if( q == AD_Q_SHORT ) 
		return gp_send(&z);
	else
		return gp_in_cmd(&z);
//	return gp_in_cmd(&z);
}

int ad_hard_reset(int q, int gp_dst) {
	cmd_send_t z = {
		.queue = q,
		.dst = gp_dst,
		.ev_handler = 0,
		.target_n = AD_TARGET_HARD_RESET,
		.cmd_n = 0x0D,
		.set_timeout = 999,
		.polling = 0,
		.data_len = 0,
	};
	return gp_in_cmd(&z);
}

int ad_turn_on(int q, int gp_dst, int subdev) {
	cmd_send_t z = {
		.queue = q,
		.dst = gp_dst,
		.ev_handler = 0,
		.target_n = AD_TARGET_TURN_ON,
		.cmd_n = 0x07,
		.set_timeout = 100,
		.polling = 0,
		.data_len = 1
	};
	z.cmd_buff[0]=subdev;
	return gp_in_cmd(&z);
}

int ad_turn_off(int q, int gp_dst) {
	cmd_send_t z = {
		.queue = q,
		.dst = gp_dst,
		.ev_handler = 0,
		.target_n = AD_TARGET_TURN_OFF,
		.cmd_n = 0x08,
		.set_timeout = 100,
		.polling = 0,
		.data_len = 0
	};
	return gp_in_cmd(&z);
}

int ad_exchange(int q, int gp_dst, int new_dst) {
	cmd_send_t z = {
		.queue = q,
		.dst = gp_dst,
		.ev_handler = 0,
		.target_n = AD_TARGET_EXCHANGE,
		.cmd_n = 0x05,
		.set_timeout = 100,
		.polling = 0,
		.data_len = 6,
		.cmd_buff = { 0x00, 0xC0, 1, 0x00, 0x01}
	};
	z.cmd_buff[5]=new_dst;
	if (gp_in_cmd(&z) > 0)
		return ad_soft_reset(q,gp_dst);
	return 0;
	
/*	if( q == AD_Q_SHORT ) 
		return gp_send(&z);
	else
		return gp_in_cmd(&z);*/
}

/*
*
*	RTC 0x00, 0xD0
*
*/
int ad_get_date(int q, int gp_dst) {
	cmd_send_t z = {
		.queue = q,
		.dst = gp_dst,
		.ev_handler = 0,
		.target_n = AD_TARGET_GET_DATE,
		.cmd_n = 0x04,
		.set_timeout = 100,
		.polling = 0,
		.data_len = 5,
		.cmd_buff = { 0x00, 0xD0, 7, 0x00, 0x00}
	};
	return gp_in_cmd(&z);
}

int ad_set_date(int q, int gp_dst, gp_date_rtc_t* d) {
	cmd_send_t z = {
		.queue = q,
		.dst = gp_dst,
		.ev_handler = 0,
		.target_n = AD_TARGET_SET_DATE,
		.cmd_n = 0x05,
		.set_timeout = 100,
		.polling = 0,
		.data_len = 5+8,
		.cmd_buff = { 0x00, 0xD0, 8, 0x00, 0x00}
	};
	
	memcpy(&z.cmd_buff[5], (void *)d, 8);
	return gp_in_cmd(&z);
}

int ad_get_cid(int q, int gp_dst, ev_cbfunc handler) {
	cmd_send_t z = {
		.queue = q,
		.dst = gp_dst,
		.ev_handler = handler,
		.target_n = AD_TARGET_GET_CID,
		.cmd_n = 0x04,
		.set_timeout = 100,
		.polling = 0,
		.data_len = 5,
		.cmd_buff = { 0x00, 0xD0, 6, 0x00, 0x0C}
	};
//	z.ev_handler=handler
	
	if( q == AD_Q_SHORT ) 
		return gp_send(&z);
	else
		return gp_in_cmd(&z);
}

int ad_get_buff_addr(int q, int gp_dst, ev_cbfunc handler) {
	cmd_send_t z = {
		.queue = q,
		.dst = gp_dst,
		.ev_handler = handler,
		.target_n = AD_TARGET_GET_BUFF_ADDR,
		.cmd_n = 0x04,
		.set_timeout = 100,
		.polling = 0,
		.data_len = 5,
		.cmd_buff = { 0x00, 0xD0, 4, 0x00, 0x08}
	};
	return gp_in_cmd(&z);
}

int ad_get_buff_data(int q, int gp_dst, uint16_t addr, size_t l) {
	cmd_send_t z = {
		.queue = q,
		.dst = gp_dst,
		.ev_handler = 0,
		.target_n = AD_TARGET_GET_BUFF_DATA,
		.cmd_n = 0x04,
		.set_timeout = 100,
		.polling = 0,
		.data_len = 5,
		.cmd_buff = { 0x02, 0xA0}
	};
	uint16_t a=htobe16(addr);
	z.cmd_buff[2]=l;
	memcpy(&z.cmd_buff[3],&a,2);
	if( q == AD_Q_SHORT ) 
		return gp_send(&z);
	else
		return gp_in_cmd(&z);
//	return gp_send(&z);
//	gp_in_cmd(&z);
}

int ad_set_buff_addr_down(int q, int gp_dst, uint16_t addr) {
	cmd_send_t z = {
		.queue = q,
		.dst = gp_dst,
		.ev_handler = 0,
		.target_n = AD_TARGET_SET_BUFF_ADDR_DOWN,
		.cmd_n = 0x05,
		.set_timeout = 100,
		.polling = 0,
		.data_len = 7,
		.cmd_buff = { 0x00, 0xD0, 2, 0x00, 0x0A}
	};
	uint16_t a=htobe16(addr);
	memcpy(&z.cmd_buff[5],&a,2);
	if( q == AD_Q_SHORT ) 
		return gp_send(&z);
	else
		return gp_in_cmd(&z);
}

int ad_reset_buff(int q, int gp_dst) {
	cmd_send_t z = {
		.queue = q,
		.dst = gp_dst,		
		.ev_handler = 0,
		.target_n = AD_TARGET_RESET_BUFF,
		.cmd_n = 0x05,
		.set_timeout = 100,
		.polling = 0,
		.data_len = 9,
		.cmd_buff = { 0x00, 0xD0, 4, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00}
//		.cmd_buff = { 0x00, 0xD0, 4, 0x00, 0x08, 0x7F, 0x90, 0x7F, 0x90}
	};
//	int i;
//	for(i=5;i<z.data_len;i++) z.cmd_buff[i]=0;
	if( q == AD_Q_SHORT ) 
		return gp_send(&z);
	else
		return gp_in_cmd(&z);
}

int ad_get_token_bound(int q, int gp_dst, ev_cbfunc handler) {
	cmd_send_t z = {
		.queue = q,
		.dst = gp_dst,
		.ev_handler = handler,
		.target_n = AD_TARGET_GET_TOKEN_BOUND,
		.cmd_n = 0x04,
		.bank = 0,
		.set_timeout = 100,
		.polling = 0,
		.data_len = 5,
		.cmd_buff = { 0x00, 0xA0, 2, 0x00, 0xBE}
	};
	
	int r;
	if( q == AD_Q_SHORT ) {
		r=gp_send(&z);
		z.bank=1;
		z.cmd_buff[0]=0x01;
		r|=gp_send(&z);
	} else {
		r=gp_in_cmd(&z);
		z.bank=1;
		z.cmd_buff[0]=0x01;
		r|=gp_in_cmd(&z);
	}
	return r;
}


int ad_set_token_bound(int q, int gp_dst, uint16_t addr) {
	cmd_send_t z = {
		.queue = q,
		.dst = gp_dst,
		.ev_handler = 0,
		.target_n = AD_TARGET_SET_TOKEN_BOUND,
		.cmd_n = 0x05,
		.bank = 0,
		.set_timeout = 100,
		.polling = 0,
		.data_len = 7,
		.cmd_buff = { 0x00, 0xA0, 2, 0x00, 0xBE}
	};
	uint16_t a=htobe16(addr);
	memcpy(&z.cmd_buff[5],&a,2);
	
	int r;
	if( q == AD_Q_SHORT ) {
		r=gp_send(&z);
		z.bank=1;
		z.cmd_buff[0]=0x01;
		r|=gp_send(&z);
	} else {
		r=gp_in_cmd(&z);
		z.bank=1;
		z.cmd_buff[0]=0x01;
		r|=gp_in_cmd(&z);
	}
	return r;
}

int ad_reset_token_bound(int q, int gp_dst) {
	return ad_set_token_bound(q, gp_dst, (uint16_t)0x00C0);
// 	cmd_send_t z = {
// 		.queue = q,
// 		.dst = gp_dst,
// 		.ev_handler = 0,
// 		.target_n = AD_TARGET_RESET_TOKEN_BOUND,
// 		.cmd_n = 0x05,
// 		.bank = 0,
// 		.set_timeout = 100,
// 		.polling = 0,
// 		.data_len = 7,
// 		.cmd_buff = { 0x00, 0xA0, 2, 0x00, 0xBE, 0x00, 0xC0}
// //		.cmd_buff = { 0x00, 0xA0, 2, 0x00, 0xBE, 0x80, 0x00}
// 	};
// 	
// 	int r;
// 	if( q == AD_Q_SHORT ) {
// 		r=gp_send(&z);
// 		z.bank=1;
// 		z.cmd_buff[0]=0x01;
// 		r|=gp_send(&z);
// 	} else {
// 		r=gp_in_cmd(&z);
// 		z.bank=1;
// 		z.cmd_buff[0]=0x01;
// 		r|=gp_in_cmd(&z);
// 	}
// 	return r;
}


int ad_get_regs(int q, int gp_dst) {
	cmd_send_t z = {
		.queue = q,
		.dst = gp_dst,
		.ev_handler = 0,
		.target_n = AD_TARGET_GET_REGS,
		.cmd_n = 0x06,
		.set_timeout = 100,
		.polling = 1,
		.data_len = 2,
		.cmd_buff = { 0x15, 0x28}
	};
	if( q == AD_Q_SHORT ) 
		return gp_send(&z);
	else
		return gp_in_cmd(&z);
}

int ad_get_token(int q, int gp_dst, uint16_t addr, int n) {
	cmd_send_t z = {
		.queue = q,
		.dst = gp_dst,
		.ev_handler = 0,
		.target_n = AD_TARGET_GET_TOKEN,
		.cmd_n = 0x04,
		.bank = 0,
		.set_timeout = 100,
		.polling = 0,
		.data_len = 5,
		.cmd_buff = { 0x00, 0xA0, 8*n, 0, 0 }
	};
	uint16_t a=htobe16(addr);
	memcpy(&z.cmd_buff[3], &a, 2);
// TODO check 	data_len
	
	int r;
	if( q == AD_Q_SHORT ) {
		r=gp_send(&z);
		z.bank=1;
		z.cmd_buff[0]=0x01;
		r|=gp_send(&z);
	} else {
		r=gp_in_cmd(&z);
		z.bank=1;
		z.cmd_buff[0]=0x01;
		r|=gp_in_cmd(&z);
	}
	return r;
}

int ad_set_token(int q, int gp_dst, uint16_t addr, gp_token_rec_t* data, int n) {
	cmd_send_t z = {
		.queue = q,
		.dst = gp_dst,
		.ev_handler = 0,
		.target_n = AD_TARGET_SET_TOKEN,
		.cmd_n = 0x05,
		.bank = 0,
		.set_timeout = 100,
		.polling = 0,
		.data_len = 5+8*n,
		.cmd_buff = { 0x00, 0xA0, 8*n, 0, 0 }
	};
	uint16_t a=htobe16(addr);
	memcpy(&z.cmd_buff[3], &a, 2);
	memcpy(&z.cmd_buff[5], (uint16_t *)data, 8*n);
// TODO check 	data_len

	int r;
	if( q == AD_Q_SHORT ) {
		r=gp_send(&z);
		z.bank=1;
		z.cmd_buff[0]=0x01;
		r|=gp_send(&z);
	} else {
		r=gp_in_cmd(&z);
		z.bank=1;
		z.cmd_buff[0]=0x01;
		r|=gp_in_cmd(&z);
	}
	return r;
}


int ad_get_times(int q, int gp_dst) {
	cmd_send_t z = {
		.queue = q,
		.dst = gp_dst,
		.ev_handler = 0,
		.target_n = AD_TARGET_GET_TIMES,
		.cmd_n = 0x04,
		.bank = 0,
		.set_timeout = 100,
		.polling = 0,
		.data_len = 5,
		.cmd_buff = { 0x00, 0xA0, 2, 0x00, 0x00}
	};
	
	int r;
	if( q == AD_Q_SHORT ) {
		r=gp_send(&z);
		z.bank=1;
		z.cmd_buff[0]=0x01;
		r|=gp_send(&z);
	} else {
		r=gp_in_cmd(&z);
		z.bank=1;
		z.cmd_buff[0]=0x01;
		r|=gp_in_cmd(&z);
	}
	return r;
}

int ad_set_times(int q, int gp_dst,gp_times_t* t) {
	cmd_send_t z = {
		.queue = q,
		.dst = gp_dst,
		.ev_handler = 0,
		.target_n = AD_TARGET_SET_TIMES,
		.cmd_n = 0x05,
		.bank = 0,
		.set_timeout = 100,
		.polling = 0,
		.data_len = 5+4,
		.cmd_buff = { 0x00, 0xA0, 2, 0x00, 0x00}
	};
	memcpy(&z.cmd_buff[5], (void *)t, 4);
	
	int r;
	if( q == AD_Q_SHORT ) {
		r=gp_send(&z);
		z.bank=1;
		z.cmd_buff[0]=0x01;
		r|=gp_send(&z);
	} else {
		r=gp_in_cmd(&z);
		z.bank=1;
		z.cmd_buff[0]=0x01;
		r|=gp_in_cmd(&z);
	}
	return r;
}



int ad_get_sched_table(int q, int gp_dst, ev_cbfunc handler) {
	cmd_send_t z = {
		.queue = q,
		.dst = gp_dst,
		.ev_handler = handler,
		.target_n = AD_TARGET_GET_SCHED,
		.cmd_n = 0x04,
		.bank = 0,
		.set_timeout = 100,
		.polling = 0,
		.data_len = 5,
		.cmd_buff = { 0x00, 0xA0, 56, 0x00, 0x78}
	};
	
	int r;
	if( q == AD_Q_SHORT ) {
		r=gp_send(&z);
		z.bank=1;
		z.cmd_buff[0]=0x01;
		r|=gp_send(&z);
	} else {
		r=gp_in_cmd(&z);
		z.bank=1;
		z.cmd_buff[0]=0x01;
		r|=gp_in_cmd(&z);
	}
	return r;
}

int ad_set_sched_rec(int q, int gp_dst, int num, gp_time_zone_t rec, ev_cbfunc handler) {
	cmd_send_t z = {
		.queue = q,
		.dst = gp_dst,
		.ev_handler = handler,
		.target_n = AD_TARGET_SET_SCHED,
		.cmd_n = 0x05,
		.bank = 0,
		.set_timeout = 100,
		.polling = 0,
		.data_len = 5 + sizeof(gp_time_zone_t),
		.cmd_buff = { 0x00, 0xA0, sizeof(gp_time_zone_t), 0x00, 0x78}
	};
//	gp_time_zone_t* p=(gp_time_zone_t*)0x0078;
	uint16_t a=htobe16( (uint16_t)(0x0078 + sizeof(gp_time_zone_t)*num));
	memcpy(&z.cmd_buff[3], &a, 2);	
	memcpy(&z.cmd_buff[5], (void *)&rec, sizeof(gp_time_zone_t));
	
	
	int r;
	if( q == AD_Q_SHORT ) {
		r=gp_send(&z);
		z.bank=1;
		z.cmd_buff[0]=0x01;
		r|=gp_send(&z);
	} else {
		r=gp_in_cmd(&z);
		z.bank=1;
		z.cmd_buff[0]=0x01;
		r|=gp_in_cmd(&z);
	}
	return r;
}

int ad_prep_load(int q, int gp_dst, int enable) {
	cmd_send_t z = {
		.queue = q,
		.dst = gp_dst,
		.ev_handler = 0,
		.target_n = AD_TARGET_PREP_LOAD,
		.cmd_n = 0x66,
		.set_timeout = 100,
		.polling = 0,
		.data_len = 1,
		.cmd_buff = {0x73}
	};
	if( enable == 0 ) z.cmd_buff[0]=0;
	if( q == AD_Q_SHORT ) 
		return gp_send(&z);
	else
		return gp_in_cmd(&z);
}
