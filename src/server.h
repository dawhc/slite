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

#define SERVER_NAME "slite v1.0"
#define GATEWAY_INTERFACE "CGI/1.1"

#define THREAD_NUMBER 16
#define EVENT_NUMBER 1024

typedef struct server {
	int port;
	char path[1024];
	char cgi_path[1024];
	int sock_fd;
	int epoll_fd;
	threadpool_t *tp;
	struct epoll_event *events;
} server_t;

/*
 * Initialize a slite server
 * server: server_t type instance
 * port: server port
 * path: static file resource path, NULL to set default path: "." 
 * cgi_path: cgi executable resource path, NULL to set default path: "cgi-bin"
 */
int server_init(server_t *server, int port, char *path, char *cgi_path);

int server_run(server_t *server);

#endif
