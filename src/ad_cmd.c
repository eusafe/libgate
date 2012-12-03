/*
 *
 * Adapter for controller of gate
 * 
 * Designed by Evgeny Byrganov <eu dot safeschool at gmail dot com> for safeschool.ru, 2012
 *  
 * $Id: ad_cmd.c 2822 2012-11-12 13:08:58Z eu $
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
	if( addr < GP_START_TICKET_BOUND ) {
		addr=devices[rec.dev].bound_token;
	}
	
	uint64_t cid2=db_get_cid_by_a(addr);
	if ( cid2 != 0 && cid2 != rec.token.cid ) {
		zprintf(3, "Conflict in db was: 0x%016llX, new: 0x%016llX for addr: 0x%04x\n", cid2, rec.token.cid, addr);
		return 0;
	}
	
	gp_token_rec_t t2;	
	memset(&t2, 0, sizeof(gp_token_rec_t));
	int2bin(t2.cid, rec.token.cid, 6, devices[rec.dev].cid_revert);
	t2.attr=rec.token.attr; 
	t2.schedule_mask=rec.token.schedule_mask;
	
//	t2.attr=0; t2.schedule_mask=0xFF; t2.schedule_mask=1;
	
	zprintf(7,"Writing  cid: 0x%012llX, dev=%d, addr=0x%04X, mask=0x%02X\n", 
		rec.token.cid, rec.dev, addr, rec.token.schedule_mask);
	ad_set_token(AD_Q_SECOND, rec.dev, addr, &t2, 1);
	
	if( addr >= devices[rec.dev].bound_token ) {
		devices[rec.dev].bound_token = addr + sizeof(gp_token_rec_t);
		gp_cfg.max_bound_token=MAX(gp_cfg.max_bound_token,devices[rec.dev].bound_token);
		ad_set_token_bound(AD_Q_SECOND, rec.dev, devices[rec.dev].bound_token);
	}
	return 1;
}

int cmd_add_tocken_to_all(ad_token_rec_t in_rec) {
	int gp_dst;
	proc_add_cid_t rec2;
	
//	printf("TEST1: rec. cid: 0x%016llX, schedule_mask: 0x%04x\n",in_rec.cid, in_rec.schedule_mask);
	db_token_rec_t* rec=db_get_token_by_c(in_rec.cid);	
	if( rec == 0) {
		rec2.token.cid=in_rec.cid;
		rec2.token.attr=in_rec.attr;
		rec2.token.schedule_mask=in_rec.schedule_mask;
		rec2.token.addr=gp_cfg.max_bound_token;
		if( db_add_token(&rec2.token) == 0){
			zprintf(3, "Fail saved rec. cid: 0x%016llX, addr: 0x%04x\n",rec2.token.cid, rec2.token.addr);
		}
		
	} else {
		memcpy(&rec2.token, rec, sizeof(db_token_rec_t));
		rec2.token.attr=in_rec.attr;
		rec2.token.schedule_mask=in_rec.schedule_mask;
	}
	
//	printf("TEST2: rec. cid: 0x%016llX, addr: 0x%04x\n",rec2.token.cid, rec2.token.addr);
	for(gp_dst=1;gp_dst<=gp_cfg.max_dev_n;gp_dst++) {
		if( devices[gp_dst].activ == 1 ) {
			rec2.dev=gp_dst;
			if( cmd_add_cid(rec2) == 0 ) return 0;
		}
	}
	return 1;
}

int cmd_add_cid_to_all(proc_add_cid_t rec) {
	int gp_dst;
	
	if( rec.token.addr < GP_START_TICKET_BOUND ) {
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
	int r= ad_exchange(AD_Q_FIRST,dev,new);
	devices[new].activ = 1;
	devices[dev].activ = 0;
	return r;
}

int cmd_get_sched(int dev) {
	return ad_get_sched_table(AD_Q_FIRST, dev,0);
}

int cmd_get_tocken(int dev, uint16_t addr) {
	return ad_get_token(AD_Q_FIRST, dev,addr,1);
}


int cmd_clear_dev(int dev) {
	gp_token_rec_t data[GP_MAX_TICKET_NUM];
	uint16_t addr=GP_START_TICKET_BOUND;
	int block_size=GP_MAX_TICKET_NUM * sizeof(gp_token_rec_t);
	
	zprintf(4, "Clearing all ticket mem for dev %u \n", dev);
	
	ad_prep_load(AD_Q_FIRST, dev, 1);
	memset(data,0x55,block_size);	
	for(addr=GP_START_TICKET_BOUND;addr < GP_MAX_TICKET_BOUND;addr+=block_size) {
		ad_set_token(AD_Q_FIRST, dev, addr, data, GP_MAX_TICKET_NUM);
//		zprintf(7, "Clearing mem for ticket for %04X\n", addr);
	}
	
	gp_cfg.max_bound_token=GP_MAX_TICKET_BOUND;
	devices[dev].bound_token=GP_MAX_TICKET_BOUND;
	ad_set_token_bound(AD_Q_FIRST, dev, devices[dev].bound_token);
	ad_prep_load(AD_Q_FIRST, dev, 0);
	
	ad_get_token_bound(AD_Q_FIRST, dev, 0);
	ad_get_date(AD_Q_FIRST, dev);
	zprintf(7, "Clearing mem for ticket for %04X\n", addr);
		
	return 1;
}

int cmd_clear_dev_to_all() {
	int gp_dst;
	
	for(gp_dst=1;gp_dst<=gp_cfg.max_dev_n;gp_dst++) {
		if( devices[gp_dst].activ == 1 ) {
			cmd_clear_dev(gp_dst);
		}
	}
	return 1;
}

int cmd_param_dev(int dev) {
	ad_get_info(AD_Q_FIRST, dev);
	ad_get_token_bound(AD_Q_FIRST, dev, 0);
	ad_get_buff_addr(AD_Q_FIRST,dev,0);
	ad_get_regs(AD_Q_FIRST, dev);
	ad_get_date(AD_Q_FIRST, dev);
	return 1;
}

int cmd_reset_dev(int dev) {
	set_rtc_date(dev);
	ad_reset_token_bound(AD_Q_FIRST, dev);
	ad_reset_buff(AD_Q_FIRST, dev);
	return 1;
}

int cmd_reload_dev(int dev) {
	
	ad_prep_load(AD_Q_FIRST, dev, 1);
	db_token_rec_t* token=db_get_next_token(1);
	zprintf(3,"db_get_next_token\n");
	
	while( token > 0 ) {
		gp_token_rec_t t2;	
		memset(&t2, 0, sizeof(gp_token_rec_t));
		int2bin(t2.cid, token->cid, 6, devices[dev].cid_revert);
		t2.attr=token->attr; 
		t2.schedule_mask=token->schedule_mask;
	

		zprintf(3,"Writing into  dev(%d) - cid: 0x%012llX, addr=0x%04X, mask=0x%02X, attr=0x%02X\n", 
			dev, token->cid, token->addr, token->schedule_mask, token->attr);
		ad_set_token(AD_Q_FIRST, dev,token-> addr, &t2, 1);
// next		
		token=db_get_next_token(0);
	}
/*	gp_cfg.max_bound_token=GP_MAX_TICKET_BOUND;
	devices[dev].bound_token=GP_MAX_TICKET_BOUND;
	ad_set_token_bound(AD_Q_FIRST, dev, devices[dev].bound_token);*/
	
	ad_prep_load(AD_Q_FIRST, dev, 0);
	return 1;
}

int cmd_saving_sched_to_all(ad_sched_rec_t r) {
	int gp_dst;
	int n=r.num -1;
	if( n < 0 || n > 6 ) {
		return 0;
	}
 	
	gp_time_zone_t t = {
		.wmask = r.wmask,
		.stub = {0,0,0}
	};
	int mi = r.begin/60;
	t.begin_hour = int2bcd(mi / 60);
	t.begin_min = int2bcd(mi % 60);
	mi = r.end/60;
	t.end_hour = int2bcd(mi / 60);
	t.end_min = int2bcd(mi % 60);
	for(gp_dst=1;gp_dst<=gp_cfg.max_dev_n;gp_dst++) {
		if( devices[gp_dst].activ == 1 ) {
			ad_set_sched_rec(AD_Q_FIRST, gp_dst, n, t, 0);
		}
	}
	return 1;

}

int cmd_saving_sched(uint16_t rule_id,	uint8_t zone_id, uint8_t schedule_mask, ad_sched_rec_t* recs, int amount) {
	int i=0;
	
	for(i=0;i<amount;i++) {
		db_add_param_sched(zone_id,recs[i]);
		zprintf(4,"N: %d :wmask: 0x%02x, begin: %d, end: %d [0x%02X]\n",  
			recs[i].num, recs[i].wmask, recs[i].begin, recs[i].end, schedule_mask);
		cmd_saving_sched_to_all(recs[i]);
	}
	db_add_param_rule(rule_id,zone_id,schedule_mask);

	return 1;
}


int cmd_get_33() {

	return 1;
}


