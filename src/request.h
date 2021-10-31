#ifndef REQUEST_H
#define REQUEST_H

#include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>
#include "util.h"
#include "server.h"

#define BUFFER_SIZE 8192
#define URI_MAXLEN 1024
#define CONTENT_MAXLEN 32768

typedef struct res_data {
	int is_keep_alive;
	int is_cgi;
	int modified, mtime;

	char fname[URI_MAXLEN << 1];
} res_data_t;

typedef struct req_data {
	int epoll_fd;
	int fd;

	server_t *server;

	res_data_t rout;
	
	char buf[BUFFER_SIZE];
	char buf_line[BUFFER_SIZE];
	ssize_t buf_idx;
	ssize_t buf_line_idx;
	ssize_t buf_line_sz;

	char method[10];
	char uri[URI_MAXLEN];
	char version[15];

	int parse_step;
	int crlf_cnt;

	char user_agent[BUFFER_SIZE];
	char accept_mime[BUFFER_SIZE];
	char cookie[BUFFER_SIZE];

	char content_type[BUFFER_SIZE];
	ssize_t content_length;
	char content_data[CONTENT_MAXLEN];
	ssize_t content_idx;

	enum { METHOD = 0, URI, VERSION, HEADER, BODY } PARSE_STEPS;

} req_data_t;


void init_req(req_data_t *r, server_t *server);

void init_res(res_data_t *r);

void close_req(req_data_t *r);

#endif
