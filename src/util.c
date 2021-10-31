#include "util.h"

/********** Epoll **********/

void epoll_op(int epoll_fd, int op, int fd, void *data, int events) {
	struct epoll_event event;
	event.events = events;
	event.data.ptr = data;
	epoll_ctl(epoll_fd, op, fd, &event);
}

void set_nonblocking(int fd) {
	int flag = fcntl(fd, F_GETFL, 0);
	if (flag == -1) return;
	flag |= O_NONBLOCK;
	fcntl(fd, F_SETFL, flag);
}

/********** Log **********/

char log_level_tag[][10] = {"INFO", "WARNING", "ERROR"};
char fmt_msg[65536];

void _log(const char *msg, int log_level, va_list args) {
	time_t log_time;
	time(&log_time);
	//char fmt_time[50];
	//strftime(fmt_time, 50, "%F %T %a", localtime(&log_time));
	vsprintf(fmt_msg, msg, args);
	fprintf(stderr, "%s<%s> %s\n", asctime(localtime(&log_time)), log_level_tag[log_level], fmt_msg);
}

void log_info(const char *msg, ...) {
	va_list args;
	va_start(args, msg);
	_log(msg, LOG_INFO, args);
	va_end(args);
}

void log_warn(const char *msg, ...) {
	va_list args;
	va_start(args, msg);
	_log(msg, LOG_WARN, args);
	va_end(args);
}

void log_error(const char *msg, ...) {
	va_list args;
	va_start(args, msg);
	_log(msg, LOG_ERROR, args);
	va_end(args);
}

/********** IO **********/

ssize_t send_s(int fd, void *data, size_t n) 
{
    size_t nleft = n;
    ssize_t nsend;
    char *bufp = (char *)data;
        
    while (nleft > 0) {
        if ((nsend = send(fd, bufp, nleft, 0)) <= 0) {
            if (errno == EINTR)  /* interrupted by sig handler return */
                nsend = 0;    /* and call send() again */
            else {
                log_error("errno == %d", errno);
				return -1;       /* errorno set by write() */
            }
        }
        nleft -= nsend;
        bufp += nsend;
    }
    
    return n;
}
