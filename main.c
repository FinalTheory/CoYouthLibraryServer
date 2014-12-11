/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 */
#include "proj.h"

sbuf_t ClientPool;

int main(int argc, char **argv)
{
    int listenfd, connfd, port, clientlen;
    struct sockaddr_in clientaddr;
    pthread_t tid;

	/* Handle SIGPIPE */
	struct sigaction sa;
	sa.sa_handler = SIG_IGN;
	sa.sa_flags = 0;
	if (sigaction(SIGPIPE, &sa, 0) == -1) {
		perror("Sigaction error!");
		exit(1);
	}

    /* Check command line args */
    if (argc != 2)
    {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    port = atoi(argv[1]);
    sbuf_init(&ClientPool, SBUFSIZE);
    listenfd = Open_listenfd(port);

	for ( int i = 0; i < MAXTHREADS; i++ ) {
		Pthread_create(&tid, NULL, ServeThread, NULL);
	}

    while (1)
    {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *)&clientlen);
        sbuf_insert(&ClientPool, connfd);
    }
	return 0;
}
/* $end tinymain */
