#ifndef _DB_LAYER_H
#define _DB_LAYER_H 1

typedef struct db_token_rec_t {
	uint64_t cid;
	uint8_t attr;
	uint8_t schedule_mask;
	uint16_t addr;
	uint64_t zone_mask;
} db_token_rec_t;

typedef struct db_token_addr_t {
	uint16_t addr;
	uint16_t zone_id;	
} db_token_addr_t;


int db_init(char* file);
int db_close();
int db_sync();
int db_add_token(db_token_rec_t* rec);

db_token_rec_t* db_get_token_by_c(uint64_t cid);
db_token_rec_t* db_get_token_by_a(uint16_t addr);
uint64_t db_get_cid_by_a(uint16_t addr);
int db_get_max_addr();

#endif /* _DB_LAYER_H  */
