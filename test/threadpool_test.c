#include "../threadpool/threadpool.h"

#include <stdio.h>


void test_task(void *n) {
	int a = 1, b = 1;
	for (int i = 0; i < *(int *)n; ++i) {
		int c = a + b;
		a = b;
		b = c;
	}
}

int main() {
	int n = 100000000;
	threadpool_t *threadpool = threadpool_init(10);
	for (int i = 0; i < 10; ++i)
		threadpool_add(threadpool, (threadjob_func_t)test_task, (void *)&n);
	//	pthread_t t;
//	pthread_create(&t, NULL, test_task, &n);
//	pthread_join(t, NULL);
	//test_task(&n);
	threadpool_destroy(threadpool, 1);
	return 0;
}
