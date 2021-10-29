// threadpool.h
// Created by CDH
// Date: 2021/10/22


#include <stdlib.h>
#include <pthread.h>

// Threadpool shutdown type 
#define IMMEDIATE_SHUTDOWN 1
#define GRACEFUL_SHUTDOWN 2

// Error type
#define UNKNOWN_ERROR -1
#define ALREADY_SHUTDOWN_ERROR -2
#define ALLOCATE_ERROR -3
#define FREE_ERROR -4

typedef void * (*threadjob_func_t)(void *args);

typedef struct threadjob {
	void				*args;
	struct threadjob	*nxt;
	threadjob_func_t job_func;
} threadjob_t;

typedef struct threadpool {
	int				thread_num;

	// The number of jobs waiting in job queue
	int				queue_size;

	// The head and tail pointer of job queue 
	threadjob_t		*head;
	threadjob_t		*tail;

	pthread_t		*thread_id;
	pthread_mutex_t lock;
	pthread_cond_t	cond;

	int				is_shutdown;
} threadpool_t;



threadpool_t * threadpool_init(int thread_num);

int threadpool_add(threadpool_t *pool, threadjob_func_t func, void *args);

int threadpool_destroy(threadpool_t *pool, int graceful_mode);

void *thread_handler(void *args);
