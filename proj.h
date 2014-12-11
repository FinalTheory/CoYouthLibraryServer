#ifndef __PROJ_H__
#define __PROJ_H__

#include "FuncWrapper.h"

#define SBUFSIZE 256
#define MAXTHREADS 256

/* $begin sbuft */
typedef struct {
    int *buf;          /* Buffer array */
    int n;             /* Maximum number of slots */
    int front;         /* buf[(front+1)%n] is first item */
    int rear;          /* buf[rear%n] is last item */
    sem_t mutex;       /* Protects accesses to buf */
    sem_t slots;       /* Counts available slots */
    sem_t items;       /* Counts available items */
} sbuf_t;
/* $end sbuft */

void sbuf_init(sbuf_t *sp, int n);
void sbuf_deinit(sbuf_t *sp);
void sbuf_insert(sbuf_t *sp, int item);
int sbuf_remove(sbuf_t *sp);


void * ServeThread(void * vargp);
void ServeClient(int fd);
void read_request(rio_t *, char *);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs, rio_t *rp);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);

#endif
