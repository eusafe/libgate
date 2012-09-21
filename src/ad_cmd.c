/*
 *
 * Adapter for controller of gate
 * 
 * Designed by Evgeny Byrganov <eu dot safeschool at gmail dot com> for safeschool.ru, 2012
 *  
 * $Id: ad_cmd.c 2735 2012-09-21 14:31:21Z eu $
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
	t2.attr=0; t2.time_zone_mask=0xFF;
	
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
			cmd_add_cid(rec);
		}
	}
	return 1;
}

int cmd_drop_cid() {

	return 1;
}
char* cmd_get_vector() {
	return gp_dev_vector();
}

char* cmd_get_bvector() {
	static char buff[1024];
	sprintf(buff,"%08llX",gp_dev_bvector());
	return buff;
}

int cmd_3() {

	return 1;
}


