#include "http.h"
#include "request.h"

header_handler_t header_handlers[] = {
	{"Connection", header_handler_connection},
	{"Content-Length", header_handler_content_length},
	{"If-Modified-Since", header_handler_if_modified_since},
	{"", header_handler_default}
}; 

kv_item content_types[] = {
	{".html", "text/html"},
    {".xml", "text/xml"},
    {".xhtml", "application/xhtml+xml"},
    {".txt", "text/plain"},
    {".rtf", "application/rtf"},
    {".pdf", "application/pdf"},
    {".word", "application/msword"},
    {".png", "image/png"},
    {".gif", "image/gif"},
    {".jpg", "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".au", "audio/basic"},
    {".mpeg", "video/mpeg"},
    {".mpg", "video/mpeg"},
    {".avi", "video/x-msvideo"},
    {".gz", "application/x-gzip"},
    {".tar", "application/x-tar"},
    {".css", "text/css"},
    {NULL ,"text/plain"}
};

int parse_request(req_data_t *r, int len, res_data_t *rout) {
	int err_code;
	
	for (int i = 0; i < len; ++i) {
		char ch = r->buf[r->buf_idx + i];

		if (r->parse_step == BODY) {
			r->content_data[r->content_idx++] = ch;
			if (r->content_idx == rout->req_content_length) {
				r->parse_step = METHOD;
				return PARSE_OK;
			}
			continue;
		}

		if (ch == '\r') {
			continue;
		} else if (ch == '\n') {
			r->crlf_cnt ++;
		} else {
			r->crlf_cnt = 0;
		}

		r->buf_line[r->buf_line_idx++] = ch;
		if (r->buf_line_idx >= r->buf_line_sz) {
			r->buf_line_sz += BUFFER_SIZE;
			r->buf_line = (char *)realloc(r->buf_line, r->buf_line_sz);
		}

		else if (ch == ' ') {
			switch (r->parse_step) {
				case METHOD:				
					r->buf_line[r->buf_line_idx - 1] = '\0';
					r->method = (char *)realloc(r->method, sizeof(char) * r->buf_line_idx);
					strcpy(r->method, r->buf_line);
					r->buf_line_idx = 0;
					r->parse_step = URI;
					break;
				case URI:				
					r->buf_line[r->buf_line_idx - 1] = '\0';
					r->uri = (char *)realloc(r->uri, sizeof(char) * r->buf_line_idx);
					strcpy(r->uri, r->buf_line);	
					r->buf_line_idx = 0;
					r->parse_step = VERSION;
					err_code = parse_uri(r->uri, rout->fname, r->server->path);
					if (err_code) return err_code;
					break;
				default:
					break;

			}
		} else if (ch == '\n') {
			switch (r->parse_step) {
				case VERSION:				
					r->buf_line[r->buf_line_idx - 1] = '\0';
					r->version = (char *)realloc(r->version, sizeof(char) * r->buf_line_idx);
					strcpy(r->version, r->buf_line);
					r->buf_line_idx = 0;
					r->parse_step = HEADER;
					break;
				case HEADER:
					if (r->crlf_cnt == 1) {
						char *colon_pos = strchr(r->buf_line, ':');
						if (colon_pos == NULL)
							return PARSE_ERROR;
						char *key_end, *val_start, *key_start = r->buf_line, *val_end = r->buf_line + r->buf_line_idx - 1;
						for (key_end = colon_pos - 1; key_end > key_start && *(key_end - 1) == ' '; --key_end);
						for (val_start = colon_pos + 1; val_start < val_end && *val_start == ' '; ++val_start);
						*key_end = *val_end = '\0';
						add_header(r, key_start, val_start);
						r->buf_line_idx = 0;
					} else {
						r->buf_line_idx = 0;
						r->buf_idx += i;
						err_code = parse_header(r, rout);
						if (err_code) return err_code;
						if (rout->req_content_length == 0) {
							r->parse_step = METHOD;
							return PARSE_OK;
						} else {
							r->content_length = rout->req_content_length;
							r->content_data = (char *)realloc(r->content_data, sizeof(char) * r->content_length);
							r->content_idx = 0;
							r->parse_step = BODY;
						}
					}
					break;
					
				case BODY:
					break;
				default:
					return PARSE_ERROR;
					break;
			}
		}	
	}
	r->buf_idx += len;
	return PARSE_WAIT;
}

void * handle_request(void *req) {
	req_data_t *r = (req_data_t *)req;

	int fd = r->fd;
	int n, len = 0;
	res_data_t rout;
	struct stat finfo;

	// log_info("New request from fd %d", fd);
	init_res(&rout);

	while(1) {
		n = recv(fd, r->buf + (r->buf_idx % BUFFER_SIZE), BUFFER_SIZE - r->buf_idx % BUFFER_SIZE, 0);
		
		if (n == 0) {
			log_warn("Read fd %d get EOF, closing...", fd);
			close_req(r);
			return NULL;
		}
		if (n < 0) {
			if (errno != EAGAIN) {
				log_error("Read fd %d get unexpected error, closing...", fd);
				close_req(r);
				return NULL;
			}
			break;
		} 

		len += n;
		
		// parse request method, uri, HTTP version and headers from request
		int status = parse_request(r, n, &rout);
		if (status != PARSE_OK && status != PARSE_WAIT) {
			if (status == PARSE_URI_TOOLONG) {
				// URI Too Long
				render_error(fd, 414, "URI Too Long");
				close_req(r);
				return NULL;
			}
			// Bad Request
			render_error(fd, 400, "Bad Request");
			close_req(r);
			return NULL;
		}
		if (status == PARSE_WAIT)
			continue;
		
		log_info("method = %s, uri = %s", r->method, r->uri);

		if (stat(rout.fname, &finfo) < 0) {
			render_error(fd, 404, "Not Found");
			close_req(r);
			return NULL;
		}

		if (S_ISDIR(finfo.st_mode)) {
			strcat(rout.fname, "/index.html");
			if (stat(rout.fname, &finfo) < 0) {
				render_error(fd, 404, "Not Found");
				close_req(r);
				return NULL;
			}
		}

		if (!(S_ISREG(finfo.st_mode)) || !(S_IRUSR & finfo.st_mode)) {
			render_error(fd, 403, "Forbidden");
			close_req(r);
			return NULL;
		}

		rout.content_length = finfo.st_size;
		rout.mtime = finfo.st_mtime;

		render(fd, rout.fname, &rout);
		
		if (!rout.is_keep_alive) {
			close_req(r);
			return NULL;
		}
		init_res(&rout);
	}

	epoll_op(r->epoll_fd, EPOLL_CTL_MOD, r->fd, req, EPOLLIN | EPOLLET | EPOLLONESHOT);
	return NULL;
}
		

int parse_header(req_data_t *r, res_data_t *rout) {
	header_list_t *ls = &r->headers;
	for (header_t *header = ls->head; header != ls->tail; header = header->nxt) {
		for (int i = 0; ; ++i) {
			header_handler_t *handler = &header_handlers[i];
			if (strcmp(header->key, handler->key) == 0 || strcmp("", handler->key) == 0) {
				(*handler->handler)(header->val, r, rout);
				break;
			}
		}
	}
	clear_header(r);
	return 0;
}
int parse_uri(char *uri, char *fname, char *root_dir) {
	if (uri == NULL)
		return -1;
	int len = strlen(uri);
	char *query_pos = strchr(uri, '?');
	char *anchor_pos = strchr(uri, '#');
	if (query_pos != NULL)
		len = len < (query_pos - uri) ? len : (query_pos - uri);
	if (anchor_pos != NULL)
		len = len < (anchor_pos - uri) ? len : (anchor_pos - uri);

	if (len > URI_MAXLEN)
		return PARSE_URI_TOOLONG;

	strcpy(fname, root_dir);

	strncat(fname, uri, len);

	return 0;
}
int render(int fd, char *fname, res_data_t *r) {
	char res_header[BUFFER_SIZE];

	int code = 200;
	const char *msg = "OK";

	char *file_pos = strrchr(fname, '/');
	char *file_suffix = strrchr(file_pos, '.');

	const char *type;
	for (int i = 0; ; ++i)
		if (content_types[i].key == NULL || strcmp(content_types[i].key, file_suffix) == 0) {
			type = content_types[i].value;
			break;
		}

	sprintf(res_header, "HTTP/1.1 %d %s\r\n" \
						"Server: slite\r\n" \
						"Content-Type: %s\r\n" \
						"Connection: close\r\n" \
						"Content-Length: %ld\r\n\r\n", 
						code, msg, type, r->content_length);
	
	send_s(fd, res_header, strlen(res_header));

	int f = open(fname, O_RDONLY);
	if (f == -1)
		return -1;
	char read_buf[BUFFER_SIZE];
	ssize_t cnt = 0;
	while (cnt < r->content_length) {
		ssize_t n = read(f, read_buf, BUFFER_SIZE);
		if(n < 0) {
			close(f);
			return -1;
		}
		send_s(fd, read_buf, n);
		cnt += n;
	}
	
	close(f);
	return 0;
}
int render_error(int fd, int err_code, const char *err_msg) {
	char res_header[BUFFER_SIZE], res_body[BUFFER_SIZE];

	sprintf(res_body, "<html>\n" \
						"<title>Error</title>\n" \
						"<body bgcolor=""ffffff"" style=""text-align:center;"">\n" \
						"<h1>Oooops!</h1>\n" \
						"<h3>An error occured.</h3>\n" \
						"<p>%d: %s</p>\n" \
						"<hr><em>Slite</em>\n" \
						"</body>" \
						"</html>" ,
						err_code, err_msg);

	sprintf(res_header, "HTTP/1.1 %d %s\r\n" \
						"Server: slite\r\n" \
						"Content-Type: text/html\r\n" \
						"Connection: close\r\n" \
						"Content-Length: %ld\r\n\r\n", 
						err_code, err_msg, strlen(res_body));

	send_s(fd, res_header, strlen(res_header));
	send_s(fd, res_body, strlen(res_body));

	return 0;
}

int header_handler_connection(char *value, req_data_t *r, res_data_t *rout) {
	(void) r;
	if (strcasecmp("keep-alive", value) == 0) {
		rout->is_keep_alive = 1;
	} else
		rout->is_keep_alive = 0;
	return 0;
}
int header_handler_content_length(char *value, req_data_t *r, res_data_t *rout) {
	(void) r;
	rout->req_content_length = atoi(value);
	return 0;
}
int header_handler_if_modified_since(char *value, req_data_t *r, res_data_t *rout) {
	(void) r;
	struct tm tm;
	if (strptime(value, "%a, %d %b %Y %H:%M:%S GMT", &tm) == NULL)
		return 0;
	time_t c_mtime = mktime(&tm);
	double diff = difftime(rout->mtime, c_mtime);
	if (fabs(diff) < 1e-6) {
		rout->modified = 0;
	} else
		rout->modified = 1;
	return 0;
}
int header_handler_default(char *value, req_data_t *r, res_data_t *rout) {
	(void) value;
	(void) r;
	(void) rout;
	return 0;
}
