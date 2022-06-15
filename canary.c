
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <string.h>

#ifndef PORT
#define PORT 10000
#endif

extern char *gets(char *buf);
extern int printflag();

void win()
{
	puts(" !!! You win !!!\n");
	printflag();
	exit(0);
}

#if defined(CANARY0)
void child()
{
	char buf[500];

	gets(buf);
	printf(buf);
	fflush(stdout);
}
#elif defined(CANARY1)
void child()
{
	char buf[500];

	fgets(buf, sizeof(buf), stdin);
	printf(buf);
	fflush(stdout);
}
#elif defined(CANARY2)
void child()
{
	char buf[500];
	int len, n;

	if (fread(&len, sizeof(len), 1, stdin) != 1) {
		fprintf(stderr, "read returned %d\n", n);
		return;
	}
	fread(buf, 1, len, stdin);
	fwrite(buf, 1, len, stdout);
}
#else
#error "Please define CANARY[012]"
#endif

int main()
{
	int lstn;
	int enable;
	struct sockaddr_in lstn_addr;

	lstn = socket(AF_INET, SOCK_STREAM, 0);
	if (lstn < 0) {
		perror("socket");
		return 1;
	}
	enable = 1;
	if (setsockopt(lstn, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) < 0) {
		perror("setsockopt");
		return 1;
	}
	bzero(&lstn_addr, sizeof(lstn_addr));

	lstn_addr.sin_family = AF_INET;
	lstn_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	lstn_addr.sin_port = htons(PORT);

	if (bind(lstn, (struct sockaddr *)&lstn_addr, sizeof(lstn_addr)) < 0) {
		perror("bind");
		return 1;
	}

	if (listen(lstn, 10) < 0) {
		perror("listen");
		return 1;
	}
	printf("Listening on port %d\n", PORT);

	signal(SIGCHLD, SIG_IGN);

	for (;;) {
		int con = accept(lstn, NULL, NULL);
		if (con < 0) {
			perror("accept");
			return 1;
		}

		switch (fork()) {
		case -1:
			perror("fork");
			return 1;
		case 0:
			printf("New connection, child %d\n", getpid());

			fflush(stdout);

			close(0);
			dup(con);
			close(1);
			dup(con);
			close(2);
			dup(con);
			close(con);
			child();
			exit(0);
			break;
		default:
			close(con);
			break;
		}
	}
	return 0;
}
