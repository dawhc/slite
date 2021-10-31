#include "server.h"

char res_path[] = "static";
char cgi_path[] = "cgi-bin";
int port = 8888;

int main() {
	server_t server;
	server_init(&server, port, res_path, cgi_path);
	return server_run(&server);
}
