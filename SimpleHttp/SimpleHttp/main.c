#include <stdio.h>
#include "Server.h"
#include <unistd.h>
#include <stdlib.h>
int main() {
	/*printf("!!!  %d\n", argc);
	if (argc < 3) {
		printf("./a.out prot path\n");
		return -1;
	}*/
	unsigned short port = atoi("10000");
	chdir("/home/lr13-2002/projects/SimpleHttp/pkl");
	puts("/home/lr13-2002/projects/SimpleHttp/pkl");
	//unsigned short port = atoi(argv[1]);
	//chdir(argv[2]);
	//printf("%u\n", port);
	//puts(argv[2]);
	int lfd = initListenLd(port);
	epollRun(lfd);
	system("pause");
	return 0;
}