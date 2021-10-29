#include "server.h"

const char *res_path = "static";
const int port = 8888;

int main() {
	server_t server;
	server_init(&server, port, res_path);
	return server_run(&server);
}
