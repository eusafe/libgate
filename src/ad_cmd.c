/*
 *
 * Adapter for controller of gate
 * 
 * Designed by Evgeny Byrganov <eu dot safeschool at gmail dot com> for safeschool.ru, 2012
 *  
 * $Id: ad_cmd.c 2737 2012-09-26 08:34:54Z eu $
 *
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/param.h>

#include "libgate.h"

int cmd_get_poll_result() {

	return 1;
}

int cmd_log_read(int gp_dst, ev_cbfunc handler) {
	return ad_get_buff_addr(AD_Q_SECOND,gp_dst, &cb_log_read );
}

int cmd_add_cid(proc_add_cid_t rec) {
	uint16_t addr=rec.token.addr;
	if( addr < GP_SPART_TICKET_BOUND ) {
		addr=devices[rec.dev].bound_token;
	}
	
	uint64_t cid2=db_get_cid_by_a(addr);
	if (  cid2 != rec.token.cid ) {
		zprintf(3, "Conflict in db was: 0x%016llX, new: 0x%016llX for addr: 0x%04x\n", cid2, rec.token.cid, addr);
		return 0;
	}
	
	gp_token_rec_t t2;	
	memset(&t2, 0, sizeof(gp_token_rec_t));
	int2bin(t2.cid, rec.token.cid, 6, devices[rec.dev].cid_revert);
	t2.attr=0; t2.schedule_mask=0xFF; t2.schedule_mask=1;
	
	zprintf(7,"Writing  cid: 0x%012llX, dev=%d, addr=0x%04X\n", 
		rec.token.cid, rec.dev, addr);
	ad_set_token(AD_Q_SECOND, rec.dev, addr, &t2, 1);
	
	if( addr >= devices[rec.dev].bound_token ) {
		devices[rec.dev].bound_token = addr + sizeof(gp_token_rec_t);
		gp_cfg.max_bound_token=MAX(gp_cfg.max_bound_token,devices[rec.dev].bound_token);
		ad_set_token_bound(AD_Q_SECOND, rec.dev, devices[rec.dev].bound_token);
	}
	return 1;
}

int cmd_add_cid_to_all(proc_add_cid_t rec) {
	int gp_dst;
	
	if( rec.token.addr < GP_SPART_TICKET_BOUND ) {
		rec.token.addr=gp_cfg.max_bound_token;
	}
	if( db_add_token(&rec.token) == 0){
		zprintf(3, "Fail saved rec. cid: 0x%016llX, addr: 0x%04x\n",rec.token.cid, rec.token.addr);
	}
	for(gp_dst=1;gp_dst<=gp_cfg.max_dev_n;gp_dst++) {
		if( devices[gp_dst].activ == 1 ) {
			rec.dev=gp_dst;
			if( cmd_add_cid(rec) == 0 ) return 0;
		}
	}
	return 1;
}

int cmd_drop_cid() {

	return 1;
}
char* cmd_get_dev_vector() {
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

uint64_t cmd_get_dev_bvector() {
	uint64_t v = (uint64_t)0;
	uint64_t b = 1;
	int i;
	
	for(i=0;i<gp_cfg.max_dev_n;i++) {
		if( devices[i+1].activ == 1 ) v |= b;
		b <<= 1;
	}
	return v;
}

int cmd_get_max_dev(int n) {
	return gp_cfg.max_dev_n;
}
int cmd_set_max_dev(int n) {
	if( n < 1 )  n=1;
	if( n > 255) n = 255;
	return gp_cfg.max_dev_n=n;
}

int cmd_get_info(int dev) {
	return ad_get_info(AD_Q_FIRST, dev);
}

int cmd_open_dev(int dev, int subdev) {
	return ad_turn_on(AD_Q_FIRST, dev, subdev);
}

int cmd_close_dev(int dev) {
	return ad_turn_off(AD_Q_FIRST, dev);
}

int cmd_exchange_dev(int dev, int new) {
	return ad_exchange(AD_Q_FIRST,dev,new);
}

int cmd_get_33() {

	return 1;
}


