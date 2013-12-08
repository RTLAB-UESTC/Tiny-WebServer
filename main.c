/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 */
#include "mylib.h"
#include "serverlib.h"
#include "threadpool.h"

extern Thread_Pool *pool;

int main(int argc, char **argv)
{
    int listenfd, connfd, port, num_threads;
    socklen_t clientlen;
    struct sockaddr_in clientaddr;

    /* Check command line args */
    if (argc != 3) {
		fprintf(stderr, "usage: %s <port> <num_threads>\n", argv[0]);
		exit(1);
    }

    port = atoi(argv[1]);
	num_threads = atoi(argv[2]);
    listenfd = Open_listenfd(port);

	if(listenfd == -1){
		perror("Open_listenfd");
		exit(-1);
	}

	TP_Init(num_threads);

    while (1) {
		clientlen = sizeof(clientaddr);
		connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); //line:netp:tiny:accept
		fprintf(stderr, "client %d connected\n", connfd);
		if(connfd > 0)
			TP_Add_Woker(doit, &connfd);
		//doit(connfd);                                             //line:netp:tiny:doit
		//Close(connfd);                                            //line:netp:tiny:close
    }
}

