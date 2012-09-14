/*
 *
 * Adapter for controller of gate
 * 
 * Designed by Evgeny Byrganov <eu dot safeschool at gmail dot com> for safeschool.ru, 2012
 *  
 * $Id: ad_cmd.c 2731 2012-09-14 14:55:56Z eu $
 *
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "libgate.h"

int cmd_get_poll_result() {

	return 1;
}

int cmd_log_read(int gp_dst, ev_cbfunc handler) {
	return ad_get_buff_addr(AD_Q_SECOND,gp_dst, &cb_log_read );
}

int cmd_add_cid(proc_add_cid_t rec) {
	int i=0, gp_dst;
	ad_turn_on(AD_Q_SECOND, rec.dev, rec.subdev);
	for(gp_dst=1;gp_dst<=gp_cfg.max_dev_n;i++,gp_dst++) {
		if( devices[gp_dst].activ == 1 ) {
			gp_token_rec_t t2;
			memset(&t2, 0, sizeof(gp_token_rec_t));
			int2bin(t2.cid, rec.cid, 6, devices[gp_dst].cid_revert);
			t2.attr=0; t2.time_zone_mask=0xFF;
			fprintf(stderr,"Writing  cid: 0x%012llX, dev=%d, bound=0x%04X\n", 
				rec.cid, gp_dst, devices[gp_dst].bound_token);
			ad_set_token(AD_Q_SECOND, gp_dst, devices[gp_dst].bound_token, &t2,1);
			devices[gp_dst].bound_token+=8;
			ad_set_token_bound(AD_Q_SECOND, gp_dst, devices[gp_dst].bound_token);
		}
	}
	return 1;
}

int cmd_drop_cid() {

	return 1;
}

int cmd_2() {

	return 1;
}

int cmd_3() {

	return 1;
}


