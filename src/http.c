#include "http.h"
#include "request.h"
#include "server.h"
#include <stdio.h>
#include <unistd.h>

header_handler_t header_handlers[] = {
	{"User-Agent", header_handler_ua},
	{"Accept", header_handler_accept},
	{"Cookie", header_handler_cookie},
	{"Connection", header_handler_connection},
	{"Content-Length", header_handler_content_length},
	{"Content-Type", header_handler_content_type},
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
		char ch = r->buf[r->buf_idx % BUFFER_SIZE + i];

		if (r->parse_step == BODY) {
			r->content_data[r->content_idx++] = ch;
			if (r->content_idx == r->content_length) {
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
			// r->buf_line = (char *)realloc(r->buf_line, r->buf_line_sz);
		}

		else if (ch == ' ') {
			switch (r->parse_step) {
				case METHOD:				
					r->buf_line[r->buf_line_idx - 1] = '\0';
					// r->method = (char *)realloc(r->method, sizeof(char) * r->buf_line_idx);
					strcpy(r->method, r->buf_line);
					r->buf_line_idx = 0;
					r->parse_step = URI;
					break;
				case URI:				
					r->buf_line[r->buf_line_idx - 1] = '\0';
					// r->uri = (char *)realloc(r->uri, sizeof(char) * r->buf_line_idx);
					strcpy(r->uri, r->buf_line);	
					r->buf_line_idx = 0;
					r->parse_step = VERSION;
					err_code = parse_uri(r, rout);
					if (err_code) return err_code;
					break;
				default:
					break;

			}
		} else if (ch == '\n') {
			switch (r->parse_step) {
				case VERSION:				
					r->buf_line[r->buf_line_idx - 1] = '\0';
					// r->version = (char *)realloc(r->version, sizeof(char) * r->buf_line_idx);
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
						for (key_end = colon_pos; key_end > key_start && *(key_end - 1) == ' '; --key_end);
						for (val_start = colon_pos + 1; val_start < val_end && *val_start == ' '; ++val_start);
						*key_end = *val_end = '\0';
						if (parse_header(key_start, val_start, r, rout))
							return PARSE_ERROR;
						r->buf_line_idx = 0;
					} else {
						r->buf_line_idx = 0;
						r->buf_idx += i;
						if (r->content_length == 0) {
							r->parse_step = METHOD;
							return PARSE_OK;
						} else {
							if (strcmp(r->method, "GET") == 0) {
								if (r->content_length > CONTENT_MAXLEN)
									return PARSE_ERROR;
								// r->content_data = (char *)realloc(r->content_data, sizeof(char) * r->content_length);
								r->content_idx = 0;
								r->parse_step = BODY;
							} else {
								// r->content_data = (char *)realloc(r->content_data, sizeof(char) * len);
								r->content_idx = len - i - 1;
								memcpy(r->content_data, r->buf + r->buf_idx + 1, len - i - 1);
								r->parse_step = METHOD;
								return PARSE_OK;
							}
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
	return PARSE_WAIT;
}

void * handle_request(void *req) {
	req_data_t *r = (req_data_t *)req;

	int fd = r->fd;
	int n;
	res_data_t *rout = &r->rout;

	// log_info("New request from fd %d", fd);
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

		// parse request method, uri, HTTP version, headers and body from request
		int status = parse_request(r, n, rout);

		r->buf_idx += n;

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

		// Parse finished
		// Try to send pages to client
		
		

		if (rout->is_cgi) {
			if (render_cgi(r, rout) == -1) {
				log_error("Failed to render CGI for fd %d", fd);
				close_req(r);
				return NULL;
			}
		} else {
			if (render(r, rout) == -1) {
				log_error("Failed to render static for fd %d", fd);
				close_req(r);
				return NULL;
			}
		}
        init_res(rout);
		if (!rout->is_keep_alive) {
            close_req(r);
            return NULL;
        }
	}
	//close_req(r);
	epoll_op(r->epoll_fd, EPOLL_CTL_MOD, r->fd, req, EPOLLIN | EPOLLET | EPOLLONESHOT);
	return NULL;
}
		

int parse_header(char *key, char *val, req_data_t *r, res_data_t *rout) {
	if (key == NULL || val == NULL || r == NULL || rout == NULL)
		return -1;

	for (int i = 0; ; ++i) {
		header_handler_t *handler = &header_handlers[i];
		if (strcasecmp(key, handler->key) == 0 || strcmp("", handler->key) == 0) {
			(*handler->handler)(val, r, rout);
			break;
		}
	}
	return 0;
}

int parse_uri(req_data_t *r, res_data_t *rout) {
	if (r == NULL || rout == NULL)
		return -1;

	int uri_len = strlen(r->uri);
	int cgi_path_len = strlen(r->server->cgi_path);

	char *uri_valid = r->uri;
	while (*uri_valid == '/') uri_valid++;
	
	if ((int)strlen(uri_valid) >= cgi_path_len && strncmp(uri_valid, r->server->cgi_path, cgi_path_len) == 0) {
		rout->is_cgi = 1;
		uri_len -= (uri_valid - r->uri);
	} else { 
		rout->is_cgi = 0;
		uri_valid = r->uri;
	}

	char *query_pos = strchr(uri_valid, '?');
	char *anchor_pos = strchr(uri_valid, '#');
	if (query_pos != NULL)
		uri_len = uri_len < (query_pos - uri_valid) ? uri_len : (query_pos - uri_valid);
	if (anchor_pos != NULL)
		uri_len = uri_len < (anchor_pos - uri_valid) ? uri_len : (anchor_pos - uri_valid);

	if (uri_len > URI_MAXLEN)
		return PARSE_URI_TOOLONG;

	if (!rout->is_cgi)
		strcpy(rout->fname, r->server->path);

	strncat(rout->fname, uri_valid, uri_len);

	return 0;
}
int render(req_data_t *r, res_data_t *rout) {
	if (r == NULL || rout == NULL)
		return -1;

	int fd = r->fd;
	char *fname = rout->fname;
	struct stat finfo;

	if (stat(fname, &finfo) < 0) {
		render_error(fd, 404, "Not Found");
		return -1;
	}

	if (S_ISDIR(finfo.st_mode)) {
		strcat(fname, "/index.html");
		if (stat(fname, &finfo) < 0) {
			render_error(fd, 404, "Not Found");
			return -1;
		}
	}

	if (!(S_ISREG(finfo.st_mode)) || !(S_IRUSR & finfo.st_mode)) {
		render_error(fd, 403, "Forbidden");
		return -1;
	}
	
	rout->mtime = finfo.st_mtime;

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
						"Server: " SERVER_NAME "\r\n" \
						"Content-Type: %s\r\n" \
						"Connection: %s\r\n" \
						"Content-Length: %ld\r\n\r\n", 
						code, msg, type, 
						rout->is_keep_alive ? "keep-alive" : "close",
						finfo.st_size);
	
	if(send_s(fd, res_header, strlen(res_header)) == -1) return -1;

	int f = open(fname, O_RDONLY, 0);
	char *faddr = mmap(NULL, finfo.st_size, PROT_READ, MAP_PRIVATE, f, 0);
	close(f);
	send_s(fd, faddr, finfo.st_size);
	munmap(faddr, finfo.st_size);

	log_info("Response %d: %s", code, msg);

	return 0;
}

int render_cgi(req_data_t *r, res_data_t *rout) {
	if (r == NULL || rout == NULL)
		return -1;
	int fd = r->fd;
	char *fname = rout->fname;
	struct stat finfo;

	if (stat(fname, &finfo) < 0) {
		render_error(fd, 404, "Not Found");
		return -1;
	}

	if (S_ISDIR(finfo.st_mode)) {
		strcat(fname, "/index.cgi");
		if (stat(fname, &finfo) < 0) {
			render_error(fd, 404, "Not Found");
			return -1;
		}
	}

	if (!(S_ISREG(finfo.st_mode)) || !(S_IRUSR & finfo.st_mode) || !(S_IXOTH & finfo.st_mode)) {
		render_error(fd, 403, "Forbidden");
		return -1;
	}

	int cgi_rpipe[2], cgi_wpipe[2];
	if (pipe(cgi_rpipe) < 0 || pipe(cgi_wpipe) < 0) {
		log_error("Failed to create pipe for CGI!");
		render_error(r->fd, 500, "Server Internal Error");
		return -1;
	}
	int pid = fork();
	if (!pid) {

		// Child process
		
		dup2(cgi_rpipe[1], STDOUT_FILENO);
		dup2(cgi_wpipe[0], STDIN_FILENO);
		//close(cgi_rpipe[0]);
		//close(cgi_wpipe[1]);
		
		// Change working directory to cgi path
		chdir(r->server->cgi_path);
		
		// Change the relative path of cgi file 
		for (fname = strchr(fname, '/'); *fname == '/'; fname++);

		char port_str[10];
		sprintf(port_str, "%d", r->server->port);
		setenv("REQUEST_METHOD", r->method, 1);
		setenv("SERVER_SOFTWARE", SERVER_NAME, 1);
		setenv("SERVER_PROTOCOL", r->version, 1);
		setenv("SERVER_PORT", port_str, 1);
		setenv("GATEWAY_INTERFACE", GATEWAY_INTERFACE, 1);
		setenv("HTTP_ACCEPT", r->accept_mime, 1);
		setenv("HTTP_USER_AGENT", r->user_agent, 1);
		setenv("HTTP_COOKIE", r->cookie, 1);
		
		if (strcmp(r->method, "GET") == 0) {
			setenv("QUERY_STRING", r->content_data, 1);	
		} else if (strcmp(r->method, "POST") == 0){
			char content_length_str[10];
			sprintf(content_length_str, "%ld", r->content_length);
			setenv("CONTENT_TYPE", r->content_type, 1);
			setenv("CONTENT_LENGTH", content_length_str, 1);
		}


		char exec_name[URI_MAXLEN << 1] = "./";
		strcat(exec_name, fname);
		execlp(exec_name, exec_name, NULL);
		exit(-1);
		
	} else {

		// Main process

		close(cgi_rpipe[1]);
		close(cgi_wpipe[0]);

		int code = 200;
		char msg[50] = "OK";
		
		// Send data to CGI executable
		
		ssize_t n;
		char res_buf[BUFFER_SIZE];
		if (strcmp(r->method, "POST") == 0) {
			// Send data remaining in buffer to CGI executable
			if (send_s(cgi_wpipe[1], r->content_data, r->content_idx) == -1) return -1;
			// Send data from remote to CGI executable
			int cnt = r->content_idx;
			while (cnt < r->content_length) {
				char *buf_start = r->buf + (r->buf_idx % BUFFER_SIZE);
				n = recv(fd, buf_start, BUFFER_SIZE - r->buf_idx % BUFFER_SIZE, 0);

				if (n == 0) {
					close(cgi_rpipe[0]);
					close(cgi_wpipe[1]);
					kill(pid, SIGKILL);
					return -1;
				}
				if (n < 0) {
					if (errno != EAGAIN) {
						close(cgi_rpipe[0]);
						close(cgi_wpipe[1]);
						kill(pid, SIGKILL);
						return -1;
					}
					continue;
				} 
				cnt += n;
				if (send_s(cgi_wpipe[1], buf_start, n) == -1) return -1;
				r->buf_idx += n;
			}	
		}
		
		close(cgi_wpipe[1]);

		sprintf(res_buf, "HTTP/1.1 %d %s\r\n" \
							"Server: " SERVER_NAME "\r\n" \
							"Connection: %s\r\n" \
							"Transfer-Encoding: chunked\r\n",
							code, msg, rout->is_keep_alive ? "keep-alive" : "close");
		
		// Transfer data from CGI executable to remote
		if (send_s(fd, res_buf, strlen(res_buf)) == -1) return -1;

		FILE *f_cgi_rpipe_r = fdopen(cgi_rpipe[0], "r");

		// Read headers from CGI executable
		while (!feof(f_cgi_rpipe_r)) {
			char *in = fgets(res_buf, BUFFER_SIZE - 1, f_cgi_rpipe_r);
			if (!in) {
				break;
			}
			if (!strcmp(in, "\r\n") || !strcmp(in, "\n")) {
				res_buf[0] = '\0';
				break;
			}
			if (send_s(fd, in, strlen(in)) == -1) return -1;
		}

		// Read body from CGI executable
		char size_buf[30];
		while (!feof(f_cgi_rpipe_r)) {
			n = fread(res_buf, 1, BUFFER_SIZE - 1, f_cgi_rpipe_r);
			if (n <= 0) {
				break;
			}
			sprintf(size_buf, "\r\n%zX\r\n", n);
			if (send_s(fd, size_buf, strlen(size_buf)) == -1) return -1;
			if (send_s(fd, res_buf, n) == -1) return -1; 
		}
		sprintf(size_buf, "\r\n0\r\n\r\n");
		if (send_s(fd, size_buf, strlen(size_buf)) == -1) return -1;
		fclose(f_cgi_rpipe_r);
		int status;
		waitpid(pid, &status, 0);
		close(cgi_rpipe[0]);

		log_info("CGI executable exited with status %d", status);
		log_info("Response %d: %s", code, msg);
	}
	return 0;
}

int render_error(int fd, int err_code, const char *err_msg) {

	log_error("Response %d: %s", err_code, err_msg);

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
						"Server: " SERVER_NAME "\r\n" \
						"Content-Type: text/html\r\n" \
						"Connection: close\r\n" \
						"Content-Length: %ld\r\n\r\n", 
						err_code, err_msg, strlen(res_body));

	if (send_s(fd, res_header, strlen(res_header)) == -1) return -1;
	if (send_s(fd, res_body, strlen(res_body)) == -1) return -1;

	return 0;
}

int header_handler_ua(char *value, req_data_t *r, res_data_t *rout) {
	(void) rout;
	//r->user_agent = (char *)realloc(r->user_agent, sizeof(char) * (strlen(value) + 1));
	strcpy(r->user_agent, value);
	return 0;
}
int header_handler_accept(char *value, req_data_t *r, res_data_t *rout) {
	(void) rout;
	//r->accept_mime = (char *)realloc(r->accept_mime, sizeof(char) * (strlen(value) + 1));
	strcpy(r->accept_mime, value);
	return 0;
}
int header_handler_cookie(char *value, req_data_t *r, res_data_t *rout) {
	(void) rout;
	//r->cookie = (char *)realloc(r->cookie, sizeof(char) * (strlen(value) + 1));
	strcpy(r->cookie, value);
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
	(void) rout;
	sscanf(value, "%ld", &r->content_length);
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

int header_handler_content_type(char *value, req_data_t *r, res_data_t *rout) {
	(void) rout;
	// r->content_type = (char *)realloc(r->content_type, sizeof(char) * (strlen(value) + 1));
	strcpy(r->content_type, value);
	return 0;
}
