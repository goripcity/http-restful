/***************************
 *文件名称：aredis.c
 *功能描述：异步redis
 *作    者：LYC
 *创建日期：2014-10-15
 *版    本：R1
 *编码格式：utf-8
 **************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <stdarg.h>
#include <getopt.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/select.h>
#include <poll.h>
#include <sys/ioctl.h>

#include <pthread.h>
#include <sys/sem.h>

#include "aredis.h"



void redis_eventread(epoll_struct *event_loop, int fd, void *data)
{
    client *c = (client *)data;
    redisAsyncHandleRead(c->context);
}


void redis_eventwrite(epoll_struct *event_loop, int fd, void *data)
{
    client *c = (client *)data;
    redisAsyncHandleWrite(c->context);
}


static void redis_addread(void *data)
{
    client *c = (client *)data;
    epoll_add_event(c->event_loop, c->rfd, EV_READABLE, redis_eventread, data);
}


static void redis_delread(void *data)
{
    client *c = (client *)data;
    epoll_del_event(c->event_loop, c->rfd, EV_READABLE);
}


static void redis_addwrite(void *data)
{
    client *c = (client *)data;
    epoll_add_event(c->event_loop, c->rfd, EV_WRITABLE, redis_eventwrite, data);
}


static void redis_delwrite(void *data)
{
    client *c = (client *)data;
    epoll_del_event(c->event_loop, c->rfd, EV_WRITABLE);
}



static void redis_cleanup(void *data)
{
    client *c = (client *)data;
    redis_delread(data);
    redis_delwrite(data);
}


void redis_attach(client *client, redisAsyncContext *ac)
{
    redisContext *c = &(ac->c);
    client->rfd = c->fd;    

    ac->ev.addRead = redis_addread;
    ac->ev.delRead = redis_delread;
    ac->ev.addWrite = redis_addwrite;
    ac->ev.delWrite = redis_delwrite;
    ac->ev.cleanup = redis_cleanup;
    ac->ev.data = client;

    return;
}
