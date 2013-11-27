#include "threadpool.h"
#include "mylib.h"

static Thread_Pool *pool = NULL;

void TP_Init(int max_thread_num)
{
	int i = 0;

	pool = (Thread_Pool*)malloc(sizeof(Thread_Pool));
	pthread_mutex_init(&(pool->queue_lock), NULL);
	pthread_cond_init(&(pool->queue_ready), NULL);
	pool->queue_head = NULL;
	pool->max_thread_num = max_thread_num;
	pool->cur_queue_size = 0;
	pool->shutdown = 0;
	pool->threads = (pthread_t*)malloc(max_thread_num * sizeof(pthread_t));

	for(i=0; i<max_thread_num; i++){		
		pthread_create(&(pool->threads[i]), NULL, Thread_Routine, &i);
	}
}

int TP_Add_Woker(void *(*func)(void *), void *arg)
{
	TP_Worker *new_worker = (TP_Worker*)malloc(sizeof(TP_Worker));
	new_worker->func = func;
	new_worker->arg = arg;
	new_worker->next = NULL;

	pthread_mutex_lock(&(pool->queue_lock));

	TP_Worker *aworker = pool->queue_head;

	if(aworker != NULL){
		while(aworker->next != NULL)
			aworker = aworker->next;
		aworker->next = new_worker;
	}
	else{
		pool->queue_head = new_worker;
	}

	assert(pool->queue_head != NULL);

	pool->cur_queue_size++;

	pthread_mutex_unlock(&(pool->queue_lock));
	pthread_cond_signal(&(pool->queue_ready));

	return 0;
}

void *Thread_Routine(void *no)
{
	TP_Worker *aworker;
	int num = *((int*)no);

	Signal(SIGPIPE, SIG_IGN);

	printf("starting no.%d thread, pid = %lu\n", num, pthread_self());

	while(1){
		pthread_mutex_lock(&(pool->queue_lock));

		while(pool->cur_queue_size==0 && !pool->shutdown){
			printf("thread(no.%d) is waiting\n", num);
			pthread_cond_wait(&(pool->queue_ready), &(pool->queue_lock));
		}
		if(pool->shutdown){
			pthread_mutex_unlock(&(pool->queue_lock));
			printf("thread(no.%d) will exit\n", num);
			pthread_exit(NULL);
		}

		printf("thread(no.%d) is starting to work\n", num);

		assert(pool->cur_queue_size != 0);
		assert(pool->queue_head != NULL);

		pool->cur_queue_size--;
		aworker = pool->queue_head;
		pool->queue_head = aworker->next;
		pthread_mutex_unlock(&(pool->queue_lock));

		(*(aworker->func))(aworker->arg);//invoke the user defined function in the thread

		free(aworker);
		aworker = NULL;
	}
}

int TP_Destroy()
{
	int i;

	if(pool->shutdown)
		return -1;
	pool->shutdown = 1;

	pthread_cond_broadcast(&(pool->queue_ready));

	for(i=0; i<pool->max_thread_num; i++){
		pthread_join(pool->threads[i], NULL);
	}
	free (pool->threads);

	TP_Worker *aworker = NULL;

	while(pool->queue_head != NULL){
		aworker = pool->queue_head;
		pool->queue_head = aworker->next;
		free(aworker);
	}

	pthread_mutex_destroy(&(pool->queue_lock));
	pthread_cond_destroy(&(pool->queue_ready));

	free(pool);

	pool = NULL;

	return 0;
}
