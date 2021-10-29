#include "request.h"

void init_req(req_data_t *r, server_t *server) {
	memset(r, 0, sizeof(req_data_t));
	r->headers.head = (header_t *)malloc(sizeof(header_t));
	r->headers.head->nxt = NULL;
	r->headers.tail = r->headers.head;
	r->buf_line = (char *)malloc(sizeof(char) * BUFFER_SIZE);
	r->buf_line_sz = BUFFER_SIZE;
	r->content_data = (char *)malloc(sizeof(char) * BUFFER_SIZE);
	r->method = (char *)malloc(sizeof(char));
	r->uri = (char *)malloc(sizeof(char));
	r->version = (char *)malloc(sizeof(char));
	r->server = server;
} 

void init_res(res_data_t *r) {
	memset(r, 0, sizeof(res_data_t));
	r->modified = 1;
}

void add_header(req_data_t *r, char *key, char *value) {
	header_t *new_header = (header_t *)malloc(sizeof(header_t));
	int key_len = strlen(key), val_len = strlen(value);
	new_header->key = (char *)malloc(sizeof(char) * (key_len + 1));
	new_header->val = (char *)malloc(sizeof(char) * (val_len + 1));
	strcpy(new_header->key, key);
	strcpy(new_header->val, value);
	new_header->nxt = r->headers.head;
	r->headers.head = new_header;
}

void clear_header(req_data_t *r) {
	header_t *iter = r->headers.head, *tail = r->headers.tail;
	while (iter != tail) {
		header_t *tmp = iter;
		iter = iter->nxt;
		free(tmp);
	}
	r->headers.head = r->headers.tail;
}

void close_req(req_data_t *r) {
	log_info("Closing connection with fd %d", r->fd);
	close(r->fd);
	free(r->buf_line);
	free(r->method);
	free(r->uri);
	free(r->version);
	free(r->content_data);
	clear_header(r);
	free(r->headers.tail);
	free(r);
}
