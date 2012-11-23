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
#include <string.h>

#include <db.h> 
#include <sys/param.h>

#include "libgate.h"

DB *db_handler;
DB *db_handler_by_addr;
DB *db_handler_param;

int db_init(char* file) {
  int ret;
  
  ret=db_create(&db_handler,NULL,0);
  if( ret != 0) {
  	return 0;
  }
  
  ret = db_handler->open(db_handler, NULL, file, "token", DB_BTREE, DB_CREATE, 0);
  if( ret != 0) {
        db_handler->err(db_handler, ret, "Database '%s' open failed.", file);
  	return 0;
  }
//  db_handler_by_addr
  ret=db_create(&db_handler_by_addr,NULL,0);
  if( ret != 0) {
  	return 0;
  }
  
  ret = db_handler_by_addr->open(db_handler_by_addr, NULL, file, "mapping", DB_BTREE, DB_CREATE, 0);
  if( ret != 0) {
        db_handler_by_addr->err(db_handler_by_addr, ret, "Database '%s' open failed.", file);
  	return 0;
  }
//   db_handler_param
  ret=db_create(&db_handler_param,NULL,0);
  if( ret != 0) {
  	return 0;
  }
  
  ret = db_handler_param->open(db_handler_param, NULL, file, "param", DB_BTREE, DB_CREATE, 0);
  if( ret != 0) {
        db_handler_param->err(db_handler_param, ret, "Database '%s' open failed.", file);
  	return 0;
  }
  
  zprintf(4, "databases opened successfully\n");
  return 1;
}

int db_close() {
  int ret;
  
  ret = db_handler->close(db_handler, 0);
  if( ret != 0) {
  	zprintf(3, "gatedb database close failed: %s\n",  db_strerror(ret));
  	return 0;
  }
  
//  db_handler_by_addr
  ret = db_handler_by_addr->close(db_handler_by_addr, 0);
  if( ret != 0) {
  	zprintf(3, "mapping database close failed: %s\n",  db_strerror(ret));
  	return 0;
  }

//   db_handler_param
  ret = db_handler_param->close(db_handler_param, 0);
  if( ret != 0) {
  	zprintf(3, "mapping database close failed: %s\n",  db_strerror(ret));
  	return 0;
  }
  
  return 1;
}

int db_sync() {
  int ret;
  
  ret = db_handler->sync(db_handler, 0);
  if( ret != 0) {
  	zprintf(7, "sync database close failed: %s\n",  db_strerror(ret));
  	return 0;
  }
  ret = db_handler_by_addr->sync(db_handler_by_addr, 0);
  if( ret != 0) {
  	zprintf(7, "sync database close failed: %s\n",  db_strerror(ret));
  	return 0;
  }
  ret = db_handler_param->sync(db_handler_param, 0);
  if( ret != 0) {
  	zprintf(7, "sync database close failed: %s\n",  db_strerror(ret));
  	return 0;
  }
  return 1;
}

/*
*
*
*
*/

int db_add_token(db_token_rec_t* rec) {
  DBT k, d;
  int ret;
  uint64_t cid = db_get_cid_by_a(rec->addr);
  
  if ( cid  != 0 &&  cid != rec->cid) {
	zprintf(2, "Conflict in db was: 0x%016llX, new:  0x%016llX for addr: 0x%04x\n", cid, rec->cid, rec->addr);
  	return 0;
  }

  rec->zone_mask=1;
  memset(&k, 0, sizeof(DBT));
  memset(&d, 0, sizeof(DBT));
  k.data=&rec->cid;
  k.size=sizeof(uint64_t);
  d.data=rec;
  d.size=sizeof(db_token_rec_t);
  
  ret=db_handler->put(db_handler,NULL,&k,&d,0);
  if( ret != 0) {
  	db_handler->err(db_handler, ret, "Put failed");
  	return 0;
  }
  
  db_token_addr_t zaddr = {
  	.addr = rec->addr,
  	.zone_id = 1
  };
  
  memset(&k, 0, sizeof(DBT));
  memset(&d, 0, sizeof(DBT));
  k.data=&zaddr;
  k.size=sizeof(db_token_addr_t);
  d.data=&rec->cid;
  d.size=sizeof(uint64_t);
  
  ret=db_handler_by_addr->put(db_handler_by_addr,NULL,&k,&d,0);
  if( ret != 0) {
  	db_handler_by_addr->err(db_handler_by_addr, ret, "Put failed");
  	return 0;
  }
  zprintf(4, "Saved rec. cid: 0x%016llX\n",rec->cid);
  db_sync();
  return 1;
}

db_token_rec_t* db_get_token_by_c(uint64_t cid) {
  DBT k, d;
  int ret;
  static db_token_rec_t rec;
  
  memset(&k, 0, sizeof(DBT));
  memset(&d, 0, sizeof(DBT));
  k.data=&cid;
  k.size=sizeof(uint64_t);
  d.data=&rec;
  d.ulen=sizeof(db_token_rec_t);
  d.flags = DB_DBT_USERMEM;
  
  zprintf(7,"Search for cid: 0x%016llX\n", cid);
  ret=db_handler->get(db_handler, NULL, &k, &d, 0);
//  if ( d.data !=  &rec) fprintf(stderr,"d.data = 0x%lx, &rec = 0x%lx\n",d.data, &rec);
  if( ret == DB_NOTFOUND) return 0;
/*  if( rec.cid  == 0 ) return 0;*/
  if( ret != 0) {
  	db_handler->err(db_handler, ret, "Get cid failed");
  	return 0;
  }
// fprintf(stderr,"Got cid: 0x%016llX for 0x%04x, size=%u\n",((db_token_rec_t*)d.data)->cid, rec.addr, d.size );
 zprintf(6,"Got cid: 0x%016llX for 0x%04x, size=%u\n",rec.cid, rec.addr, d.size );
  
  return &rec;
}

uint64_t db_get_cid_by_a(uint32_t addr) {
  DBT k, d;
  int ret;
  uint64_t cid;
  
  db_token_addr_t zaddr = {
  	.addr = addr,
  	.zone_id = 1
  };
  
  memset(&k, 0, sizeof(DBT));
  memset(&d, 0, sizeof(DBT));
  k.data=&zaddr;
  k.size=sizeof(db_token_addr_t);
  d.data=&cid;
  d.ulen=sizeof(uint64_t);
  d.flags = DB_DBT_USERMEM;
  
  ret=db_handler_by_addr->get(db_handler_by_addr,NULL,&k,&d,0);
  if( ret == DB_NOTFOUND) return 0;
  if( ret != 0) {
  	db_handler_by_addr->err(db_handler_by_addr, ret, "Get2 cid failed");
  	return 0;
  }
  return cid;
}

db_token_rec_t* db_get_token_by_a(uint32_t addr) {
  uint64_t cid=db_get_cid_by_a(addr);
  
  return db_get_token_by_c(cid);
}


int db_get_max_addr() {
  DBT k, d;
  DBC* cur=0;
  int max_addr=0x00C0 - sizeof(gp_token_rec_t);
  
  memset(&k, 0, sizeof(DBT));
  memset(&d, 0, sizeof(DBT));
  db_handler_by_addr->cursor(db_handler_by_addr, NULL, &cur, 0);
  
  while ( cur->c_get(cur, &k, &d, DB_NEXT) == 0) {
  	max_addr=MAX(max_addr,*((uint16_t*)k.data));
  	zprintf(8,"check cid: 0x%016llX, addr 0x%04x\n", *(uint64_t*)d.data, *((uint16_t*)k.data));
  }
  zprintf(4,"max addr 0x%04x\n", max_addr);

  return max_addr;
}

db_token_rec_t*  db_get_next_token(int fisrt) {
  static DBT k, d;
  static DBC* cur=0;
  uint32_t flag=DB_NEXT;
  
  if(cur == 0) {
	memset(&k, 0, sizeof(DBT));
	memset(&d, 0, sizeof(DBT));
	db_handler->cursor(db_handler, NULL, &cur, 0);
  }
  if(fisrt) flag=DB_FIRST;
    
  if( cur->c_get(cur, &k, &d, flag) == 0) {
  	return (db_token_rec_t*)d.data;
  }
  
  return 0;
}

int db_add_param(db_param_key_t* rec,void* buff, int len) {
  DBT k, d;
  int ret;
//  uint64_t cid = db_get_cid_by_a(rec->addr);
  

  memset(&k, 0, sizeof(DBT));
  memset(&d, 0, sizeof(DBT));
  k.data=rec;
  k.size=sizeof(db_param_key_t);
  d.data=buff;
  d.size=len;
  
  ret=db_handler_param->put(db_handler_param,NULL,&k,&d,0);
  if( ret != 0) {
  	db_handler_param->err(db_handler_param, ret, "Put failed");
  	return 0;
  }
  
  zprintf(4, "Saved param (%u) \n", rec->param_id);
  db_sync();
  return 1;
}

int db_add_param_rule(uint16_t rule_id,	uint8_t zone_id, uint8_t schedule_mask) {
  db_param_key_t k = { 
//  	.rule = { .rule_id = rule_id, .zone_id = zone_id },
  	.param_id = PARAM_ID_RULE
  	};
  k.rule.rule_id=rule_id;
  k.rule.zone_id=zone_id;
  db_param_rule_value_t d = { .schedule_mask = schedule_mask };
  
  return db_add_param(&k,&d,sizeof(db_param_rule_value_t));
}

int db_add_param_sched(uint8_t zone_id, ad_sched_rec_t d)  {
  db_param_key_t k = { 
  	.param_id = PARAM_ID_SCHED
  	};
  k.sched.zone_id=zone_id;
  k.sched.num = d.num;
  
  return db_add_param(&k,&d,sizeof(db_param_sched_value_t));
}
