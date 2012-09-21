#ifndef _GP_PROCESSOR_H
#define _GP_PROCESSOR_H 1

// struct  {
// 
// };
// extern struct

typedef struct {
	int dev;
	int subdev;
	db_token_rec_t  token;
	int ev;
} proc_add_cid_t;

int process_port_input(int reply);

int cb_get_poll_result(int reply);
int cb_log_read(int reply);

#endif /* _GP_PROCESSOR_H */