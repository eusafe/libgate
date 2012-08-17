#ifndef _GP_LAYER_H
#define _GP_LAYER_H 1


#define GP_SPART_TICKET_BOUND 0x00C0
#define GP_SPART_TICKET_SIZE 8

#pragma pack(push,1)

typedef union gp_cid_t {
	uint8_t cid_b[6];
	uint64_t cid;
} gp_cid_t;

typedef union gp_addr1_t {
	uint8_t buff[2];
	uint16_t bound;
} gp_addr1_t;

// typedef union gp_get_addr2_t {
// 	uint8_t buff[4];
// 	struct {
// 		uint16_t bound_up;
// 		uint16_t bound_down;
// 	};
// } gp_get_addr2_t;


typedef struct gp_addr2_t {
	uint16_t bound_up;
	uint16_t bound_down;
} gp_addr2_t;

// typedef union gp_event_t {
// 	uint8_t buff[8];
// 	struct {
// 		uint8_t ev_code;
// 		uint16_t addr;
// 		struct {
// 			uint8_t date[5];
// 		};
// 	};
// } gp_event_t
// __attribute__ ((aligned (1)))
// ;

// #pragma pack(1)
// //#pragma aling=1
// __attribute__ ((aligned (1))) 
// __attribute__ ((aligned (1)))
//__attribute__ ((packed))


typedef struct gp_reg_cid_t {
	uint8_t cid[6];
	uint8_t stub1[9];
	uint8_t addr;
	uint8_t stub2[2];
	uint8_t addr2;
	uint8_t type;
	uint8_t ev;
} gp_reg_cid_t;


typedef struct gp_port_status_t {
// P0	
	uint8_t d_0_0:1;
	uint8_t d_1_0:1;
	uint8_t d_0_1:1;
	uint8_t d_1_1:1;
	uint8_t led:2;
	uint8_t sw2:1;
	uint8_t sw1:1;
// P1
	uint8_t sa5:1;
	uint8_t sa6:1;
	uint8_t sa7:1;
	uint8_t sa8:1;
	uint8_t stub_one:4;
// P2
	uint8_t ger1:1;
	uint8_t ger2:1;
	uint8_t sensor1:1;
	uint8_t sensor2:1;
	uint8_t k1:1;
	uint8_t k2:1;
	uint8_t k3:1;
	uint8_t k4:1;
// P3
	uint8_t stub_2:2;
	uint8_t intr:1;
	uint8_t stub_3:3;
	uint8_t sa9:1;
	uint8_t sa10:1;
} gp_port_status_t;


// typedef struct gp_port_event_t {
// 	uint8_t 
// } gp_port_event_t;

typedef struct gp_token_rec_t {
	uint8_t cid[6];
	union {
		uint8_t attr;
		struct {
			uint8_t io:1;
			uint8_t cid_type:2;
			uint8_t ohrana1:1;
			uint8_t ohrana2:1;
			uint8_t stub_1:2;
			uint8_t anti:1;
		};
	};
	uint8_t time_zone_mask;
} gp_token_rec_t;

typedef struct gp_port_cmd4_t {
	uint8_t addr;
	uint8_t mem;
	uint8_t len;
	uint16_t begin;
} gp_port_cmd4_t;


typedef struct gp_info_t {
	uint8_t ver:4;
	uint8_t model:4;
	uint8_t if_type1:4;
	uint8_t if_type2:4;
	int stub1:3;
	uint8_t if_type_n:1;
	int stub2:4;
	uint8_t release;
} gp_info_t;

typedef struct gp_times_t {
	uint8_t open_door;
	uint8_t ctrl_close;
	uint8_t ctrl_open;
	uint8_t timeout_confirm;
} gp_times_t;

typedef struct gp_event_t {
	uint8_t ev_code;
	uint16_t addr;
	struct {
		uint8_t mon;
		uint8_t day;
		uint8_t hour;
		uint8_t min;
		uint8_t sec;
	} ;
} gp_event_t;

typedef struct gp_date_rtc_t {
	uint8_t sec:7;
	uint8_t stop:1;
	uint8_t min;
	uint8_t hour;
	uint8_t wday;
	uint8_t day;
	uint8_t mon;
	uint8_t year;
	uint8_t ctrl;
} gp_date_rtc_t;


typedef struct gp_time_zone_t {
	uint8_t wmask;
	uint8_t begin_hour;
	uint8_t begin_min;
	uint8_t end_hour;
	uint8_t end_min;
	uint8_t stub[3];
} gp_time_zone_t;


#pragma pack(pop)
int set_rtc_date(int dev);
struct tm* get_rtc_date(gp_date_rtc_t* d);
struct tm* get_rtc_date(gp_date_rtc_t* d);
struct tm* get_ev_date(gp_event_t* d);
time_t get_ev_time(gp_event_t* d);

#endif /* _GP_LAYER_H */

