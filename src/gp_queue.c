/*
 *
 * Adapter for controller of gate
 * 
 * Designed by Evgeny Byrganov <eu dot safeschool at gmail dot com> for safeschool.ru, 2012
 *  
 * $Id: gp_queue.c 2692 2012-08-17 13:36:51Z eu $
 *
 */

#include <sys/queue.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "libgate.h"

// queue for 
TAILQ_HEAD(gp_circleq, gp_queue) gp_head, gp_head2;
struct gp_queue {
	TAILQ_ENTRY(gp_queue) entries;
	cmd_send_t cmd;
};

int count=0;

//sem_t	available_sem;
//static	pthread_mutex_t		tp_mtx; /* for coarse-grained locking */

int gp_in_cmd(cmd_send_t* z) {
	struct gp_queue *en1;
	
//	if(TAILQ_EMPTY(&gp_head)) fprintf(stderr, "gp_head is empted \n");;
	en1 = malloc(sizeof(struct gp_queue));
	if( en1 == NULL ) {
		fprintf(stderr, "Got malloc NULL \n");
		return 0;
	}
//	fprintf(stderr, "malloc 0x%lX \n",en1);
	memset(en1, 0, sizeof(struct gp_queue));
	memcpy(&en1->cmd, z, sizeof(struct cmd_send_t));
//	fprintf(stderr, "memcpy ok \n");
	
	
//	pthread_mutex_lock(&tp_mtx);
//	TAILQ_INSERT_HEAD(&gp_head, en1, entries);
	if( z->queue ==  AD_Q_FIRST) {
		TAILQ_INSERT_TAIL(&gp_head, en1, entries);
	} else {
		TAILQ_INSERT_TAIL(&gp_head2, en1, entries);
	}
//	pthread_mutex_unlock(&tp_mtx);
	

//	fprintf(stderr, "TAILQ_INSERT_HEAD ok,  malloc 0x%lX\n",en1);
	return 1;
}

int gp_del_cmd(struct gp_queue *enp) {
//	struct gp_queue *en1, *enp;
	
	TAILQ_REMOVE(&gp_head, enp, entries);
	return 0;
}

int gp_put_cmd() {
	struct gp_queue *enp=0;
//	fprintf(stderr, "gp_put_cmd 1 working \n");

//qdump();
	
//	pthread_mutex_lock(&tp_mtx);
	if( ! TAILQ_EMPTY(&gp_head)) {

		enp=TAILQ_FIRST(&gp_head);
//		enp=TAILQ_LAST(&gp_head,gp_circleq);
//		fprintf(stderr, "gp_put_cmd 2 ok, enp: 0x%lX \n",enp);
		TAILQ_REMOVE(&gp_head, enp, entries);
	
	} else if( ! TAILQ_EMPTY(&gp_head2)) {
		enp=TAILQ_FIRST(&gp_head2);
		TAILQ_REMOVE(&gp_head2, enp, entries);
	}
//	pthread_mutex_unlock(&tp_mtx);
	if( enp == 0 ) return 0;
	
//	fprintf(stderr, "gp_put_cmd 3 ok \n");
	int r=gp_send(&(enp->cmd));
//qdump();
	free(enp);
//	fprintf(stderr, "send cmd N %d, free 0x%X \n",++count,enp);
//	if(TAILQ_EMPTY(&gp_head)) fprintf(stderr, "gp_head is empted aster removed \n");
//	fprintf(stderr, "TAILQ_REMOVE ok \n");
	return r;
}

int qdump() {
	struct gp_circleq*  p=&gp_head;
	struct gp_queue* i;
	
	fprintf(stderr,"qdump, head: 0x%X, tqh_first=0x%X, tqh_last=0x%X\n",
		(uintptr_t)p,
		(uintptr_t)p->tqh_first,
		(uintptr_t)p->tqh_last
	);
	TAILQ_FOREACH(i,p,entries) {
		fprintf(stderr,"for 0x%X,   next: 0x%X, prev: 0x%X\n",
			(uintptr_t)i, 
			(uintptr_t)i->entries.tqe_next,
			(uintptr_t)i->entries.tqe_prev
		);
	}	
	return 1;
}

int gp_queue_init() {
//	pthread_mutex_init(&tp_mtx, 0);
	TAILQ_INIT(&gp_head);
	TAILQ_INIT(&gp_head2);

	return 1;
}
