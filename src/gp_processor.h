#ifndef _GP_PROCESSOR_H
#define _GP_PROCESSOR_H 1

// struct  {
// 
// };
// extern struct

typedef struct proc_add_cid_t{
	int dev;
	int subdev;
	db_token_rec_t  token;
	int ev;
} proc_add_cid_t;

typedef struct  proc_info_t{
	int dev;
	uint8_t ver;
	uint8_t model;
	uint8_t release;
	uint8_t if_type;
} proc_info_t;

typedef struct  proc_event_t {
	int dev;
	int subdev;
	int ev;
	uint16_t addr;
	uint64_t cid;
	time_t ts;
	struct tm* ct;
	int dup;
} proc_event_t;

int process_port_input(int reply);

int cb_get_poll_result(int reply);
int cb_log_read(int reply);

//extern int (*cb_dev_info)(proc_info_t rec);
extern int (*cb_push_new_cid_handler)(proc_add_cid_t rec);
extern int (*cb_dev_info_handler)(proc_info_t rec);
extern int (*cb_dev_ev_handler)(proc_event_t rec);


#endif /* _GP_PROCESSOR_H */