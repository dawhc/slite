#include "server.h"
#include "util.h"
#include "http.h"

int server_init(server_t *server, int port, char *path, char *cgi_path) {

	server->port = port;

	if (path != NULL)
		strcpy(server->path, path);
	else
		strcpy(server->path, ".");

	if (cgi_path != NULL)
		strcpy(server->cgi_path, cgi_path);
	else
		strcpy(server->cgi_path, "cgi-bin");
	
	// Init socket
	if ((server->sock_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		log_error("Failed to initialize local socket");
		return -1;
	}
	
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(port);

	if (bind(server->sock_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
		log_error("Failed to bind local socket fd");
		return -1;
	}
	
	if (listen(server->sock_fd, 1024) == -1) {
		log_error("Failed to start socket listen");
		return -1;
	}

	// Non-block mode
	set_nonblocking(server->sock_fd);

	// Init epoll
	server->events = (struct epoll_event *)malloc(sizeof(struct epoll_event) * EVENT_NUMBER);
	if (server->events == NULL) {
		log_error("Failed to allocate memory for events");
		return -1;
	}

	if ((server->epoll_fd = epoll_create1(0)) == -1) {
		log_error("Failed to create epoll fd");
		return -1;
	}
	

	// Init threadpool
	if ((server->tp = threadpool_init(THREAD_NUMBER)) == NULL) {
		log_error("Failed to initialize threadpool");
		return -1;
	}

	
	// Add socket fd to epoll
	req_data_t *server_data = (req_data_t *)malloc(sizeof(req_data_t));
	init_req(server_data, server);
	server_data->fd = server->sock_fd;
	server_data->epoll_fd = server->epoll_fd;
	epoll_op(server->epoll_fd, EPOLL_CTL_ADD, server->sock_fd, server_data, EPOLLIN | EPOLLET);

	return 0;
}

int server_run(server_t *server) {
	log_info("Server starts at port %d", server->port);
	log_info("fd=%d, resource path=%s, thread number=%d", server->sock_fd, server->path, THREAD_NUMBER);

	signal(SIGPIPE, SIG_IGN);

	while (1) {
		int event_num = epoll_wait(server->epoll_fd, server->events, EVENT_NUMBER, -1);
		for (int i = 0; i < event_num; ++i) {
			req_data_t *event_data = (req_data_t *)server->events[i].data.ptr;
			int fd = event_data->fd;

			if (fd == server->sock_fd) {

				struct sockaddr_in client_addr;
				socklen_t client_addr_size = sizeof(client_addr);
                while (1) {
                    int client_fd = accept(fd, (struct sockaddr *) &client_addr, &client_addr_size);
                    if (client_fd < 0) {
                        if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
                            break;
                        } else {
                            log_error("Failed to accept client");
                            break;
                        }
                    }

                    log_info("New conection with fd: %d", client_fd);
                    req_data_t *conn_data = (req_data_t *) malloc(sizeof(req_data_t));
                    init_req(conn_data, server);
                    conn_data->fd = client_fd;
                    conn_data->epoll_fd = server->epoll_fd;
                    epoll_op(server->epoll_fd, EPOLL_CTL_ADD, client_fd, conn_data, EPOLLIN | EPOLLET | EPOLLONESHOT);

                    set_nonblocking(client_fd);
                }
			} else {
				if ((server->events[i].events & (EPOLLERR | EPOLLHUP)) || 
						!(server->events[i].events & EPOLLIN)) {
					log_error("Bad epoll events with fd: %d, closing...", fd);
					close(fd);
					continue;
				}

				threadpool_add(server->tp, handle_request, server->events[i].data.ptr);
			}
		}
	}

	return 0;
}  
