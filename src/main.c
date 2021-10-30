#include "server.h"

const char *res_path = "static";
const char *cgi_path = "cgi-bin";
const int port = 8888;

int main() {
	server_t server;
	server_init(&server, port, res_path, cgi_path);
	return server_run(&server);
}
