#ifndef SERVER_H
#define SERVER_H

#include <string.h>
#include <signal.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "threadpool.h"
#include "util.h"

#define THREAD_NUMBER 8
#define EVENT_NUMBER 1024

typedef struct server {
	int port;
	char path[1024];
	int sock_fd;
	int epoll_fd;
	threadpool_t *tp;
	struct epoll_event *events;
} server_t;

int server_init(server_t *server, int port, const char *path);

int server_run(server_t *server);

#endif
