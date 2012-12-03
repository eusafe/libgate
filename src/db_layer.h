#ifndef _DB_LAYER_H
#define _DB_LAYER_H 1

#pragma pack(push,1)

typedef struct db_token_rec_t {
	uint64_t cid;
	uint8_t attr;
	uint8_t schedule_mask;
	uint32_t addr;
	uint64_t zone_mask;
} db_token_rec_t;

typedef struct db_token_addr_t {
	uint32_t addr;
	uint16_t zone_id;	
} db_token_addr_t;

typedef struct db_token_key_t {
	uint64_t cid;
	uint8_t zone_id;	
} db_token_key_t;

typedef struct db_token_value_t {
	uint8_t attr;
	uint16_t rule_id;
//	uint8_t schedule_mask;
	uint32_t addr;
} db_token_value_t;

// Common param
typedef struct db_param_cfg_key_t {
	char name[12];
} db_param_cfg_key_t;

typedef struct db_param_cfg_value_t {
	union {
		int value_n;
		char value_s[32];
	};
} db_param_cfg_value_t;

// rile & zone mapping
typedef struct db_param_rule_key_t {
	uint16_t rule_id;
	uint8_t zone_id;	
} db_param_rule_key_t;

typedef struct db_param_rule_value_t {
	uint8_t schedule_mask;
//	uint8_t stub[3];
} db_param_rule_value_t;

// Schedule
typedef struct db_param_sched_key_t {
	uint8_t zone_id;	
	uint8_t num;
} db_param_sched_key_t;

/*
typedef struct db_param_sched_value_t {
	ad_sched_rec_t	sched;
} db_param_sched_value_t;
*/
typedef struct ad_sched_rec_t db_param_sched_value_t;

// Common
typedef struct db_param_key_t {
	int param_id;
	union {
//		char buff[1];
		db_param_cfg_key_t cfg;
		db_param_rule_key_t rule;
		db_param_sched_key_t sched;
	};	
} db_param_key_t;
#pragma pack(pop)


#define PARAM_ID_CFG	1
#define PARAM_ID_RULE	2
#define PARAM_ID_SCHED	3
//#define PARAM_ID_

int db_init(char* file);
int db_close();
int db_sync();
int db_add_token(db_token_rec_t* rec);

db_token_rec_t* db_get_token_by_c(uint64_t cid);
db_token_rec_t* db_get_token_by_a(uint32_t addr);
uint64_t db_get_cid_by_a(uint32_t addr);
int db_get_max_addr();
db_token_rec_t*  db_get_next_token(int fisrt);

int db_add_param_rule(uint16_t rule_id,	uint8_t zone_id, uint8_t schedule_mask);
uint8_t db_get_param_rule(uint16_t rule_id, uint8_t zone_id);

//int db_add_param_sched(uint8_t zone_id, ad_sched_rec_t d);
//int db_add_param_sched(uint8_t zone_id, ad_sched_rec_t d);

#endif /* _DB_LAYER_H  */
