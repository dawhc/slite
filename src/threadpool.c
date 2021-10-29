// threadpool.c
// Created by CDH
// Date: 2021/10/22


#include "threadpool.h"

// Initialize a threadpool, return the pointer of the threadpool 
// thread_num: the number of threads in the threadpool 
threadpool_t * threadpool_init(int thread_num) {
	threadpool_t *pool = (threadpool_t *)malloc(sizeof(threadpool_t));
	if (pool == NULL)
		return NULL;

	// Init parameters
	pool->thread_num = 0;
	pool->queue_size = 0;

	// Allocate new memory for thread array and job queue
	pool->thread_id = (pthread_t *)malloc(sizeof(pthread_t) * thread_num);
	pool->head = (threadjob_t *)malloc(sizeof(threadjob_t));
	pool->tail = pool->head;

	if (pool->thread_id == NULL || pool->head == NULL) { 
		free(pool); 
		return NULL; 
	}

	// Init first job in job queue
	pool->head->job_func = NULL;
	pool->head->args = NULL;
	pool->head->nxt = NULL;

	// Initialize mutex & condition
	if (pthread_mutex_init(&(pool->lock), NULL) != 0) {
		free(pool);
		return NULL;
	}
	if (pthread_cond_init(&(pool->cond), NULL) != 0) {
		free(pool);
		return NULL;
	}

	// Create threads
	for (int i = 0; i < thread_num; ++i) {

		// A thread in threadpool is failed to create;
		if (pthread_create(&(pool->thread_id[i]), NULL, thread_handler, (void *)pool) != 0) {
			threadpool_destroy(pool, 0);
			return NULL;
		}
		pool->thread_num++;
	}

	pool->is_shutdown  = 0;
	
	return pool;
}

// Add a job to threadpool 
// pool: the target threadpool
// func: job function with the type void * (*)(*)
// args: arguments of job function
int threadpool_add(threadpool_t *pool, threadjob_func_t func, void *args) {
	if (pool == NULL || func == NULL)
		return UNKNOWN_ERROR;
	if (pthread_mutex_lock(&(pool->lock)) != 0)
		return UNKNOWN_ERROR;
	if (pool->is_shutdown)
		return ALREADY_SHUTDOWN_ERROR;

	// Allocate new memory for a new job
	threadjob_t *new_job = (threadjob_t *)malloc(sizeof(threadjob_t));
	if (new_job == NULL)
		return ALLOCATE_ERROR;

	// Setup new job
	new_job->job_func = func;
	new_job->args = args;
	
	// Add new job to job queue
	new_job->nxt = NULL;
	pool->head->nxt = new_job;
	pool->head = new_job;
	pool->queue_size++;

	// Send condition signal
	pthread_cond_signal(&(pool->cond));

	if (pthread_mutex_unlock(&(pool->lock)) != 0)
		return UNKNOWN_ERROR;

	return 0;
}

// Destroy this threadpool
// pool: threadpool to destroy
// graceful_mode: set value with 1 to wait for all jobs finished, or 0 to destroy immediately

int threadpool_destroy(threadpool_t *pool, int graceful_mode) {
	if (pool == NULL)
		return UNKNOWN_ERROR;
	if (pool->is_shutdown)
		return ALREADY_SHUTDOWN_ERROR;

	pool->is_shutdown = (graceful_mode) ? GRACEFUL_SHUTDOWN : IMMEDIATE_SHUTDOWN;

	if (pthread_cond_broadcast(&(pool->cond)) != 0)
		return UNKNOWN_ERROR;

	// Free threads
	for (int i = 0; i < pool->thread_num; ++i)
		if (pthread_join(pool->thread_id[i], NULL) != 0)
			return FREE_ERROR;

	// Free mutex and condition 
	pthread_mutex_destroy(&(pool->lock));
	pthread_cond_destroy(&(pool->cond));

	// Free thread array
	if (pool->thread_id)
		free(pool->thread_id);

	// Free job queue
	while (pool->tail->nxt) {
		threadjob_t *tmp = pool->tail->nxt;
		pool->tail->nxt = pool->tail->nxt->nxt;
		free(tmp);
	}

	return 0;
}

// Single thread handle function
// args: pointer of threadpool which contains this thread
void *thread_handler(void *args) {
	if (args == NULL)
		return NULL;
	threadpool_t *pool = (threadpool_t *)args;
	threadjob_t *job;
	while (1) {
		pthread_mutex_lock(&(pool->lock));
		while (pool->queue_size == 0 && !(pool->is_shutdown))
			pthread_cond_wait(&(pool->cond), &(pool->lock));

		// shutdown mode
		if (pool->is_shutdown == IMMEDIATE_SHUTDOWN)
			break;
		else if (pool->is_shutdown == GRACEFUL_SHUTDOWN && pool->queue_size == 0)
			break;

		// new job mode
		job = pool->tail->nxt;
		if (job == NULL) {
			pthread_mutex_unlock(&(pool->lock));
			continue;
		}

		if (job == pool->head)
			pool->head = pool->tail;

		pool->tail->nxt = job->nxt;
		pool->queue_size--;
		pthread_mutex_unlock(&(pool->lock));
		(*(job->job_func))(job->args);
		free(job);
	}
	pthread_mutex_unlock(&(pool->lock));
	pthread_exit(NULL);
	return NULL;

}
