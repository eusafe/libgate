#ifndef _GP_PROCESSOR_H
#define _GP_PROCESSOR_H 1

// struct  {
// 
// };
// extern struct

typedef struct {
	int dev;
	int subdev;
	uint64_t cid;
	int ev;
} proc_add_cid_t;

int process_port_input(int reply);

int cb_get_poll_result(int reply);
int cb_log_read(int reply);

#endif /* _GP_PROCESSOR_H */