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
DB *db_handler2;

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
//  db_handler2=db_handler;
  ret=db_create(&db_handler2,NULL,0);
  if( ret != 0) {
  	return 0;
  }
  
  ret = db_handler2->open(db_handler2, NULL, file, "mapping", DB_BTREE, DB_CREATE, 0);
  if( ret != 0) {
        db_handler2->err(db_handler2, ret, "Database '%s' open failed.", file);
  	return 0;
  }
  fprintf(stderr, "databases opened successfully\n");
  return 1;
}

int db_close() {
  int ret;
  
  ret = db_handler->close(db_handler, 0);
  if( ret != 0) {
  	fprintf(stderr, "gatedb database close failed: %s\n",  db_strerror(ret));
  	return 0;
  }
  
  ret = db_handler2->close(db_handler2, 0);
  if( ret != 0) {
  	fprintf(stderr, "mapping database close failed: %s\n",  db_strerror(ret));
  	return 0;
  }
  return 1;
}

int db_sync() {
  int ret;
  
  ret = db_handler->sync(db_handler, 0);
  if( ret != 0) {
  	fprintf(stderr, "sync database close failed: %s\n",  db_strerror(ret));
  	return 0;
  }
  ret = db_handler2->sync(db_handler2, 0);
  if( ret != 0) {
  	fprintf(stderr, "sync database close failed: %s\n",  db_strerror(ret));
  	return 0;
  }
  return 1;
}



int db_add_token(db_token_rec_t* rec) {
  DBT k, d;
  int ret;
  uint64_t cid = db_get_cid_by_a(rec->addr);
  
  if ( cid  != 0 &&  cid != rec->cid) {
	fprintf(stderr, "Conflict in db was: 0x%016llX, new:  0x%016llX for addr: 0x%04x\n", cid, rec->cid, rec->addr);
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
  
  ret=db_handler2->put(db_handler2,NULL,&k,&d,0);
  if( ret != 0) {
  	db_handler2->err(db_handler2, ret, "Put failed");
  	return 0;
  }
  fprintf(stderr, "Saved rec. cid: 0x%016llX\n",rec->cid);
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
  
  fprintf(stderr,"Search for cid: 0x%016llX\n", cid);
  ret=db_handler->get(db_handler, NULL, &k, &d, 0);
//  if ( d.data !=  &rec) fprintf(stderr,"d.data = 0x%lx, &rec = 0x%lx\n",d.data, &rec);
  if( ret == DB_NOTFOUND) return 0;
/*  if( rec.cid  == 0 ) return 0;*/
  if( ret != 0) {
  	db_handler->err(db_handler, ret, "Get cid failed");
  	return 0;
  }
// fprintf(stderr,"Got cid: 0x%016llX for 0x%04x, size=%u\n",((db_token_rec_t*)d.data)->cid, rec.addr, d.size );
 fprintf(stderr,"Got cid: 0x%016llX for 0x%04x, size=%u\n",rec.cid, rec.addr, d.size );
  
  return &rec;
}

uint64_t db_get_cid_by_a(uint16_t addr) {
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
  
  ret=db_handler2->get(db_handler2,NULL,&k,&d,0);
  if( ret == DB_NOTFOUND) return 0;
  if( ret != 0) {
  	db_handler2->err(db_handler2, ret, "Get2 cid failed");
  	return 0;
  }
  return cid;
}

db_token_rec_t* db_get_token_by_a(uint16_t addr) {
  uint64_t cid=db_get_cid_by_a(addr);
  
  return db_get_token_by_c(cid);
}


int db_get_max_addr() {
  DBT k, d;
  DBC* cur;
  int max_addr=0x00C0 - sizeof(gp_token_rec_t);
  
  memset(&k, 0, sizeof(DBT));
  memset(&d, 0, sizeof(DBT));
  db_handler2->cursor(db_handler2, NULL, &cur, 0);
  
  while ( cur->get(cur, &k, &d, DB_NEXT) == 0) {
  	max_addr=MAX(max_addr,*((uint16_t*)k.data));
  	fprintf(stderr,"check cid: 0x%016llX, addr 0x%04x\n", *(uint64_t*)d.data, *((uint16_t*)k.data));
  }
  fprintf(stderr,"max addr 0x%04x\n", max_addr);

  return max_addr;
}
