#ifndef __NETWORK_H
#define __NETWORK_H

#include "cstr.h"
#include <hiredis/async.h>

#define EV_OK 0
#define EV_ERR -1

#define EV_NONE 0
#define EV_READABLE 1
#define EV_WRITABLE 2

#define RECVBUF 1024
#define SENDBUF 8092

typedef struct epoll_strcut_s epoll_struct;
typedef void callback(epoll_struct *event_loop, int fd, void *data);

struct file_event {
    short mask;
    void *data;
    callback *read_callback;
    callback *write_callback;
};


typedef struct epoll_strcut_s {
    int epfd;
    struct file_event *file_events;
    struct epoll_event *events;
} epoll_struct;


typedef struct client_s {
    int fd;
    int send_pos;          
    int rfd;
    cstr sdata;
    cstr recvdata;
    epoll_struct *event_loop;
    redisAsyncContext *context;
} client;


int server_start();
int epoll_add_event(epoll_struct *event_loop, int fd, int mask, callback *cb, void *data);
void epoll_del_event(epoll_struct *event_loop, int fd, int mask);
void reply_callback(epoll_struct *event_loop, int fd, void *data);

#endif /* _NETWORK_H */
