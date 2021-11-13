
#ifndef UTIL_H
#define UTIL_H

#include <sys/epoll.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <stdarg.h>

#define LOG_INFO 0
#define LOG_WARN 1
#define LOG_ERROR 2

void epoll_op(int epoll_fd, int op, int fd, void *data, int events);
void set_nonblocking(int fd);
void _log(const char *msg, int log_level, va_list args);
void log_info(const char *msg, ...);
void log_warn(const char *msg, ...);
void log_error(const char *msg, ...);
ssize_t send_s(int fd, void *data, size_t n);

#endif
