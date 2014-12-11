#include "proj.h"

extern sbuf_t ClientPool;

void * ServeThread(void * vargp)
{
	Pthread_detach(pthread_self());

	while (1)
	{
		int connfd = sbuf_remove(&ClientPool);
		fprintf(stderr, "Thread %llu handling request %d.\n", (unsigned long long int)pthread_self(), connfd);
		ServeClient(connfd);
		Close(connfd);
		fprintf(stderr, "Thread %llu's request %d ended.\n", (unsigned long long int)pthread_self(), connfd);
	}
}

/*
 * ServeClient - handle one HTTP request/response transaction
 */
/* $begin ServeClient */
void ServeClient(int fd)
{
    int is_static;
    struct stat sbuf;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char filename[MAXLINE], cgiargs[MAXLINE];
    rio_t rio;

	cgiargs[0] = 0;
    /* Read request line and headers */
    Rio_readinitb(&rio, fd);
    Rio_readlineb(&rio, buf, MAXLINE);
    sscanf(buf, "%s %s %s", method, uri, version);

    if ( strcasecmp(method, "GET") == 0 ) {
		is_static = parse_uri(uri, filename, cgiargs);
    } else if ( strcasecmp(method, "POST") == 0 ) {
    	is_static = parse_uri(uri, filename, cgiargs);
    } else {
    	clienterror(fd, method, "501", "Not Implemented", "Server does not implement this method");
        return;
    }

    if (stat(filename, &sbuf) < 0)
    {
        clienterror(fd, filename, "404", "Not found",
                    "Tiny couldn't find this file");
        return;
    }

    if (is_static)   /* Serve static content */
    {
        if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode))
        {
            clienterror(fd, filename, "403", "Forbidden",
                        "Tiny couldn't read the file");
            return;
        }
        serve_static(fd, filename, sbuf.st_size);
    }
    else   /* Serve dynamic content */
    {
        if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode))
        {
            clienterror(fd, filename, "403", "Forbidden",
                        "Tiny couldn't run the CGI program");
            return;
        }
        serve_dynamic(fd, filename, cgiargs, &rio);
    }
}
/* $end ServeClient */


/*
 * serve_static - copy a file back to the client
 */
/* $begin serve_static */
void serve_static(int fd, char *filename, int filesize)
{
    int srcfd;
    char *srcp, filetype[MAXLINE], buf[MAXBUF];

    /* Send response headers to client */
    get_filetype(filename, filetype);
    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
    sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
    sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
    Rio_writen(fd, buf, strlen(buf));

    /* Send response body to client */
    srcfd = Open(filename, O_RDONLY, 0);
    srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
    Close(srcfd);
    Rio_writen(fd, srcp, filesize);
    Munmap(srcp, filesize);
}


/*
 * serve_dynamic - run a CGI program on behalf of the client
 */
/* $begin serve_dynamic */
void serve_dynamic(int fd, char *filename, char *cgiargs, rio_t *rp)
{
    char buf[MAXLINE], *emptylist[] = { NULL };

    /* Return first part of HTTP response */
    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Server: Tiny Web Server\r\n");
    Rio_writen(fd, buf, strlen(buf));

    if (Fork() == 0) { /* child */
        /* Real server would set all CGI vars here */
        read_request(rp, cgiargs);
        setenv("QUERY_STRING", cgiargs, 1);
        fprintf(stderr, "%s: %s\r\n", "QUERY_STRING", cgiargs);
        Dup2(fd, STDOUT_FILENO);         /* Redirect stdout to client */
        Execve(filename, emptylist, environ); /* Run CGI program */
    }
    Wait(NULL); /* Parent waits for and reaps child */
}
/* $end serve_dynamic */
