#ifndef _AD_TARGET_H
#define _AD_TARGET_H 1


#define AD_Q_SHORT	0
#define AD_Q_FIRST	1
#define AD_Q_SECOND	2

#define AD_TARGET_NULL		0
#define AD_TARGET_INFO		1
#define AD_TARGET_SOFT_RESET	2
#define AD_TARGET_HARD_RESET	3
#define AD_TARGET_TURN_ON	4
#define AD_TARGET_TURN_OFF	5
#define AD_TARGET_EXCHANGE	6

#define AD_TARGET_GET_REGS		10

#define AD_TARGET_GET_CID		101
#define AD_TARGET_GET_BUFF_ADDR		102
#define AD_TARGET_SET_BUFF_ADDR_DOWN	103
#define AD_TARGET_GET_BUFF_DATA		104
#define AD_TARGET_RESET_BUFF		105
#define AD_TARGET_RESET_TOKEN_BOUND	106
#define AD_TARGET_GET_TOKEN_BOUND	107
//#define AD_TARGET_

// RTC date
#define AD_TARGET_GET_DATE		121
#define AD_TARGET_SET_DATE		122

// Info
#define AD_TARGET_GET_TIMES		201
#define AD_TARGET_SET_TIMES		202

//#define AD_TARGET_
//#define AD_TARGET_

int ad_get_info(int q, int gp_dst);
int ad_soft_reset(int q, int gp_dst);
int ad_hard_reset(int q, int gp_dst);
int ad_turn_on(int q, int gp_dst, int subdev);
int ad_turn_off(int q, int gp_dst);
int ad_exchange(int q, int gp_dst, int new_dst);
int ad_get_cid(int q, int gp_dst, ev_cbfunc handler);

int ad_get_buff_addr(int q, int gp_dst, ev_cbfunc handler);
int ad_get_buff_data(int q, int gp_dst, uint16_t addr, size_t l);
int ad_reset_buff(int q, int gp_dst);
int ad_set_buff_addr_down(int q, int gp_dst, uint16_t addr);

int ad_get_token_bound(int q, int gp_dst, ev_cbfunc handler);
int ad_set_token_bound(int q, int gp_dst, uint16_t addr);
int ad_reset_token_bound(int q, int gp_dst);
int ad_set_token(int q, int gp_dst, uint16_t addr, gp_token_rec_t* data, int n);

int ad_get_regs(int q, int gp_dst);
int ad_get_times(int q, int gp_dst);
int ad_set_times(int q, int gp_dst,gp_times_t* t);

int ad_get_date(int q, int gp_dst);
int ad_set_date(int q, int gp_dst, gp_date_rtc_t* d);

#endif /* _LIBGATE_H */
