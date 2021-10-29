#ifndef REQUEST_H
#define REQUEST_H

#include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>
#include "util.h"
#include "server.h"

#define BUFFER_SIZE 4096
#define URI_MAXLEN 4096
#define CONTENT_MAXLEN 1048576

typedef struct header_item {
	char *key;
	char *val;
	struct header_item *nxt;
} header_t;

typedef struct header_list {
	header_t *head, *tail;
	int sz;
} header_list_t;

typedef struct res_data {
	int is_keep_alive;
	int modified, mtime;
	ssize_t req_content_length;
	ssize_t content_length;

	char fname[URI_MAXLEN << 1];
} res_data_t;

typedef struct req_data {
	int epoll_fd;
	int fd;

	server_t *server;
	
	char buf[BUFFER_SIZE];
	char *buf_line;
	ssize_t buf_idx;
	ssize_t buf_line_idx;
	ssize_t buf_line_sz;

	char *method;
	char *uri;
	char *version;

	int parse_step;
	int crlf_cnt;

	header_list_t headers;

	ssize_t content_length;
	char *content_data;
	ssize_t content_idx;

	enum { METHOD = 0, URI, VERSION, HEADER, BODY } PARSE_STEPS;

} req_data_t;


void init_req(req_data_t *r, server_t *server);

void init_res(res_data_t *r);

void close_req(req_data_t *r);

void add_header(req_data_t *r, char *key, char *val);

void clear_header(req_data_t *r);

#endif
