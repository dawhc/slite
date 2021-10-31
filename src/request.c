#include "request.h"
#include <sys/socket.h>

void init_req(req_data_t *r, server_t *server) {
	init_res(&r->rout);
	r->buf_idx = 0;
	r->buf_line_idx = r->buf_line_sz = 0;
	r->parse_step = METHOD;
	r->crlf_cnt = 0;
	r->content_length = 0;
	r->content_idx = 0;
	/*
	r->buf_line = (char *)malloc(sizeof(char) * BUFFER_SIZE);
	r->buf_line_sz = BUFFER_SIZE;
	r->content_data = (char *)malloc(sizeof(char) * BUFFER_SIZE);
	r->content_type = (char *)malloc(sizeof(char));
	r->user_agent = (char *)malloc(sizeof(char));
	r->accept_mime = (char *)malloc(sizeof(char));
	r->method = (char *)malloc(sizeof(char));
	r->uri = (char *)malloc(sizeof(char));
	r->version = (char *)malloc(sizeof(char));
	*/
	r->server = server;
} 

void init_res(res_data_t *r) {
	r->is_cgi = 0;
    r->is_keep_alive = 0;
    r->fname[0] = '\0';
	r->modified = 1;
}

void close_req(req_data_t *r) {
	log_info("Closing connection with fd %d", r->fd);
	close(r->fd);
	/*
	free(r->buf_line);
	free(r->method);
	free(r->uri);
	free(r->version);
	free(r->user_agent);
	free(r->accept_mime);
	free(r->content_data);
	free(r->content_type);
	*/
	free(r);
}
