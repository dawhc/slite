#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
int main() {
	chdir("cgi-bin");
	execlp("./test.py", "./test.py", NULL);
	exit(-1);
}
