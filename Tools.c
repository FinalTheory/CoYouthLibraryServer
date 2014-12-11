#include "proj.h"

const int HeaderNameNum = 4;

char HeaderNames[][80] = {
	"Content-Length:",
	"Cookie:",
	"User-Agent:",
	"Content-Type:"
};
char EnvNames[][80] = {
	"CONTENT_LENGTH",
	"HTTP_COOKIE",
	"HTTP_USER_AGENT",
	"CONTENT_TYPE"
};

/*
 * read_requesthdrs - read and parse HTTP request headers
 */
/* $begin read_requesthdrs */
void read_request(rio_t *rp, char * cgiargs)
{
	char ch_bak;
	int HeaderLength, ContentLength = 0;
    char buf[MAXLINE], HeaderContent[MAXLINE];

    Rio_readlineb(rp, buf, MAXLINE);
    while(strcmp(buf, "\r\n"))
    {
    	/* Read and output request header */
		for ( int i = 0; i < HeaderNameNum; i++ ) {
			HeaderLength = strlen(HeaderNames[i]);
			ch_bak = buf[HeaderLength];
			buf[HeaderLength] = 0;
			if ( strcasecmp(buf, HeaderNames[i]) == 0 ) {
				strcpy(HeaderContent, buf + HeaderLength + 1);
				setenv(EnvNames[i], HeaderContent, 1);
				fprintf(stderr, "%s: %s", EnvNames[i], HeaderContent);
				if ( i == 0 ) sscanf(HeaderContent, "%d", &ContentLength);
			}
			buf[HeaderLength] = ch_bak;
		}
		Rio_readlineb(rp, buf, MAXLINE);
    }
	if ( ContentLength != 0 ) {
		/* Even if this is a post request, there is also maybe cgiargs */
		Rio_readnb(rp, buf, ContentLength);
		buf[ContentLength] = 0;
		/* Finally get all the post data */
		strcat(cgiargs, "&");
		strcat(cgiargs, buf);
	}
    return;
}
/* $end read_requesthdrs */


/*
 * parse_uri - parse URI into filename and CGI args
 *             return 0 if dynamic content, 1 if static
 */
/* $begin parse_uri */
int parse_uri(char *uri, char *filename, char *cgiargs)
{
    char *ptr;

    if (!strstr(uri, "cgi-bin") && uri[strlen(uri)-1] != '/') {  /* Static content */
        strcpy(cgiargs, "");
        strcpy(filename, "./www/html");
        strcat(filename, uri);
        return 1;
    } else {  /* Dynamic content */
        ptr = index(uri, '?');
        if (ptr) {
            strcpy(cgiargs, ptr+1);
            *ptr = '\0';
        } else {
        	strcpy(cgiargs, "");
        }
        strcpy(filename, "./www");
        strcat(filename, uri);
        if (uri[strlen(uri)-1] == '/')
            strcat(filename, "cgi-bin/LibServer.py");
		//fprintf(stderr, "%s\n", filename);
        return 0;
    }
}
/* $end parse_uri */


/*
 * get_filetype - derive file type from file name
 */
void get_filetype(char *filename, char *filetype)
{
    if (strstr(filename, ".html"))
        strcpy(filetype, "text/html");
	else if (strstr(filename, ".css"))
        strcpy(filetype, "text/css");
    else if (strstr(filename, ".gif"))
        strcpy(filetype, "image/gif");
    else if (strstr(filename, ".jpg"))
        strcpy(filetype, "image/jpeg");
    else if (strstr(filename, ".ogg"))
        strcpy(filetype, "video/ogg");
	else if (strstr(filename, ".mp4"))
        strcpy(filetype, "video/mp4");
	else
		strcpy(filetype, "text/plain");
}
/* $end serve_static */


/* Create an empty, bounded, shared FIFO buffer with n slots */
/* $begin sbuf_init */
void sbuf_init(sbuf_t *sp, int n)
{
    sp->buf = Calloc(n, sizeof(int));
    sp->n = n;                       /* Buffer holds max of n items */
    sp->front = sp->rear = 0;        /* Empty buffer iff front == rear */
    Sem_init(&sp->mutex, 0, 1);      /* Binary semaphore for locking */
    Sem_init(&sp->slots, 0, n);      /* Initially, buf has n empty slots */
    Sem_init(&sp->items, 0, 0);      /* Initially, buf has zero data items */
}
/* $end sbuf_init */

/* Clean up buffer sp */
/* $begin sbuf_deinit */
void sbuf_deinit(sbuf_t *sp)
{
    Free(sp->buf);
}
/* $end sbuf_deinit */

/* Insert item onto the rear of shared buffer sp */
/* $begin sbuf_insert */
void sbuf_insert(sbuf_t *sp, int item)
{
    P(&sp->slots);                          /* Wait for available slot */
    P(&sp->mutex);                          /* Lock the buffer */
    sp->buf[(++sp->rear)%(sp->n)] = item;   /* Insert the item */
    V(&sp->mutex);                          /* Unlock the buffer */
    V(&sp->items);                          /* Announce available item */
}
/* $end sbuf_insert */

/* Remove and return the first item from buffer sp */
/* $begin sbuf_remove */
int sbuf_remove(sbuf_t *sp)
{
    int item;
    P(&sp->items);                          /* Wait for available item */
    P(&sp->mutex);                          /* Lock the buffer */
    item = sp->buf[(++sp->front)%(sp->n)];  /* Remove the item */
    V(&sp->mutex);                          /* Unlock the buffer */
    V(&sp->slots);                          /* Announce available slot */
    return item;
}
/* $end sbuf_remove */
/* $end sbufc */

