#ifndef _AD_CMD_H
#define _AD_CMD_H 1

int cmd_add_cid(proc_add_cid_t rec);

char* cmd_get_dev_vector();
uint64_t cmd_get_dev_bvector();

typedef struct ad_token_rec_t {
	uint64_t cid;
	uint8_t attr;
	uint8_t schedule_mask;
	uint64_t zone_mask;
} ad_token_rec_t;

typedef struct ad_sched_rec_t {
//	uint16_t rule_id;
//	uint8_t zone_id;
	int num;
	uint8_t wmask;
	int begin;
	int end;
} ad_sched_rec_t;

#endif /* _AD_CMD_H */
