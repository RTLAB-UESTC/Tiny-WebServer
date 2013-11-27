#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <pthread.h>

typedef struct Worker{
	void *(*func)(void *arg);
	void *arg;
	struct Worker *next;
}TP_Worker;

typedef struct{
	pthread_mutex_t queue_lock;
	pthread_cond_t	queue_ready;

	TP_Worker *queue_head;

	int shutdown;

	pthread_t *threads;

	int max_thread_num;

	int cur_queue_size;
}Thread_Pool;

void TP_Init(int max_thread_num);

int TP_Add_Woker(void *(*func)(void *), void *no);

void *Thread_Routine(void *arg);

int TP_Destroy();

#endif
