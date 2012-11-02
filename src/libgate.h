/*
 *
 * Adapter for controller of gate
 * 
 * Designed by Evgeny Byrganov <eu dot safeschool at gmail dot com> for safeschool.ru, 2012
 *  
 * $Id: libgate.h 2774 2012-11-01 12:28:32Z eu $
 *
 */


#ifndef _LIBGATE_H
#define _LIBGATE_H 1

#include <stdint.h>
#include <stdlib.h>


//#pragma PACK(1)

// Gate port
#define GP_INIT 0xFA
#define GP_END	0xF5
#define GP_ESC	0xFF

// Device type.
#define GP_CTRL 0x01
#define GP_MAIN 0x7F

// ACK/NACK and other replay
#define GP_REPLY 0x55
#define GP_REPLY_ACK 0x55
#define GP_REPLY_NACK 0xAA

#define PROC_REPLY_ACK		1
#define PROC_REPLY_NACK		128
#define PROC_REPLY_EEPROM_ERR	129


//#define GP_CMD_ 1

#define GP_PORT_SEND_BUF_LENGTH 130
#define GP_PORT_READ_BUF_LENGTH 135

// #define 
#define SOCKET_RETRY_TIMEOUT    15       // 5 seconds

#define int2bcd(x)  ((((x)/10) << 4) + (x)%10)
#define bcd2int(x)  ((((x) & 0xF0) >> 4)*10  +  ((x) & 0x0F))
#define tv2ms(x) ((x.tv_sec%100000)* 1000 + x.tv_usec/1000)

typedef int (*ev_cbfunc)(int st);

typedef struct cmd_send_t  {
	uint8_t target_n;
	ev_cbfunc ev_handler;
	int bank;
	int set_timeout; /* in mil sec */
	int polling;
	int queue;
	size_t data_len;
	uint8_t dst;
	uint8_t cmd_n;
	uint8_t cmd_buff[120];
} cmd_send_t;


#include "gp_layer.h"
#include "ad_target.h"
#include "db_layer.h"
#include "gp_processor.h"
#include "ad_cmd.h"
#include "gp_util.h"

// Util
uint8_t crc8_xor(uint8_t* p, int l);
int memcpy_esc(uint8_t* d, uint8_t* s, int l);
int memcpy_unesc(uint8_t* d, uint8_t* s, int l);
int bin2hex(uint8_t* d ,uint8_t* s, size_t len, int revert);

int long2hex(uint8_t* d, uint64_t a);
uint64_t bin2int (uint8_t* s, size_t len, int revert);
void int2bin(uint8_t* d, uint64_t a, size_t len, int revert);

uint64_t gp_dev_bvector();
char* gp_dev_vector();


// int set_rtc_date(int dev);
// struct tm* get_rtc_date(gp_date_rtc_t* d);
// struct tm* get_rtc_date(gp_date_rtc_t* d);
// struct tm* get_ev_date(gp_event_t* d);
// time_t get_ev_time(gp_event_t* d);

//int open_port(char* path, int speed);

// gp_layer.c
int gp_send(cmd_send_t* cmd);
int gp_init ();

// gp_queue.c
struct gp_queue;
int gp_in_cmd(cmd_send_t* z);
int gp_del_cmd(struct gp_queue *enp);
int gp_put_cmd();
int qdump();
int gp_queue_init();

extern int debug;
extern void zprintf(int, const char *, ...);

#endif /*_LIBGATE_H  */

