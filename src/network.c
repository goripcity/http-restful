/***************************
 *文件名称：network.c
 *功能描述：服务器网络
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
#include <netinet/tcp.h>
#include <sys/epoll.h>

#include <pthread.h>
#include <sys/sem.h>

#include "network.h"
#include "config.h"
#include "aredis.h"
#include "logic.h"


#define MAX_EVENTS 1000
#define USE_LINGER 1



void client_callback(epoll_struct *event_loop, int fd, void *data);


void redis_connected(const redisAsyncContext *c, int status) {
    client *client = c->ev.data;
    if (status != REDIS_OK){
        client->context = NULL;
        log_error("Redis connect error");
        epoll_add_event(client->event_loop, client->fd, EV_READABLE, client_callback, client);
        return;
    }

    set_linger(client->rfd, 0, NULL);
    log_debug("Redis connection [%d] for fd = %d", client->rfd, client->fd);
    epoll_add_event(client->event_loop, client->fd, EV_READABLE, client_callback, client);
}


client * client_new(epoll_struct *ev, int fd)
{
    int size = sizeof(client);
    client *c = malloc(size);

    memset(c, 0, size);
    c->fd = fd;
    c->event_loop = ev;
    c->context = redisAsyncConnect(g_config.rip, g_config.rport);
    c->recvdata = cstrbuf(RECVBUF);
    c->sdata = cstrbuf(SENDBUF);

    redis_attach(c, c->context);

    redisAsyncSetConnectCallback(c->context, redis_connected);

    return c;
}

void client_free(client *c)
{
    if (c->sdata) cstrfree(c->sdata);
    if (c->recvdata) cstrfree(c->recvdata);
    if (c->context) {
        redisAsyncDisconnect(c->context);
        log_debug("free c->context");
    }

    free(c);
    
    return;
}



int tcp_listen(char *ip, int port)
{
    int sockfd;
    int flag = 1;
    struct sockaddr_in srvaddr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char*)&flag, sizeof(flag)) < 0){
        log_error( "Error: socket set reuseaddr failed");
        return -1;
    }

    bzero(&srvaddr, sizeof(srvaddr));

    srvaddr.sin_family = AF_INET;
    srvaddr.sin_port = htons(port);

    if (ip == NULL){
        srvaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    }
    else{
        if (inet_pton(AF_INET, ip, &srvaddr.sin_addr) == 0){
            log_error("Error: inet_pton failed");
            return -1;
        }
    }

    if (bind(sockfd, (struct sockaddr *)&srvaddr, sizeof(srvaddr)) == -1){
        log_error("Error: bind failed");
        return -1;
    }

    if (listen(sockfd, 256) == -1){
        log_error("Error: listen failed.");
        return -1;
    }

    log_debug("Server %s:%d init ok ... ", ip, port);

    return sockfd;
}



void set_nonblock(int fd)
{
    long flag = 0;

    if (fd < 0) {
        return;
    }

    flag = fcntl(fd, F_GETFL, 0);
    flag |= O_NONBLOCK;
    fcntl(fd, F_SETFL, flag);
}



void sock_keep_alive(const int sockfd)
{
    int keepAlive = 1;      /* 开启keepalive属性 */
    int keepIdle = 360;     /* 如该连接在360秒内没有任何数据往来,则进行探测 */
    int keepInterval = 5;   /* 探测时发包的时间间隔为5 秒 */
    int keepCount = 3;      /* 探测尝试的次数.如果第1次探测包就收到响应了,则后2次的不再发.*/

    if (setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepAlive, sizeof(keepAlive)) != 0) {
        log_error("-----[sock_keep_alive]-----:%s", strerror(errno));
        return;
    }
    if (setsockopt(sockfd, SOL_TCP, TCP_KEEPIDLE, (void*)&keepIdle, sizeof(keepIdle)) != 0) {
        log_error("-----[sock_keep_alive]-----:%s", strerror(errno));
        return;
    }
    if (setsockopt(sockfd, SOL_TCP, TCP_KEEPINTVL, (void *)&keepInterval, sizeof(keepInterval)) != 0) {
        log_error("-----[sock_keep_alive]-----:%s", strerror(errno));
        return;
    }

    if (setsockopt(sockfd, SOL_TCP, TCP_KEEPCNT, (void *)&keepCount, sizeof(keepCount)) != 0) {
        log_error("-----[sock_keep_alive]-----:%s", strerror(errno));
        return;
    }
    return;
}



void set_error(char *err, const char *fmt, ...)
{
    va_list ap; 

    if (!err){
       return;
    }   

    va_start(ap, fmt);
    vsnprintf(err, BUFSIZ, fmt, ap);
    va_end(ap);
}


int _set_linger(int fd, int onoff, int time, char *err)
{
    struct linger cfg;
    cfg.l_onoff = onoff;
    cfg.l_linger = time;
    
    if (setsockopt(fd, SOL_SOCKET, SO_LINGER, &cfg, sizeof(cfg)) == -1) 
    {   
        set_error(err, "setsockopt SO_LINGER: %s", strerror(errno));
        return -1; 
    }   
    return 0;
}


int set_linger(int fd, int time, char *err)
{
    return _set_linger(fd, 1, time, err);
    
}
    

int ignore_linger(int fd, char *err)
{
    return _set_linger(fd, 0, 0, err);
}



int epoll_struct_create(epoll_struct *event_loop) 
{   
    int i;
    event_loop->epfd = epoll_create(1024); 
    if (event_loop->epfd == -1) {
    	log_error("epoll_create failed");
        return -1;
    }

    event_loop->events = malloc(sizeof(struct epoll_event) * MAX_EVENTS);

    event_loop->file_events = malloc(sizeof(struct file_event) * MAX_EVENTS * 10);
    for (i=0; i< MAX_EVENTS * 10; i++);
        event_loop->file_events[i].mask = EV_NONE;

    return 0;
}


int epoll_add_event(epoll_struct *event_loop, int fd, int mask, callback *cb, void *data)
{
    struct epoll_event ee;
    struct file_event *fe = &event_loop->file_events[fd];
    int op = fe->mask == EV_NONE ? EPOLL_CTL_ADD : EPOLL_CTL_MOD;

    ee.events = 0;

    if (mask & EV_READABLE) {
        ee.events |= EPOLLIN;
        fe->read_callback = cb;
    }
    else {
        ee.events |= EPOLLOUT;
        fe->write_callback = cb;
    }

    fe->mask |= mask;
    if (data) fe->data = data;
   
    ee.data.u64 = 0; 
    ee.data.fd = fd;

    if (epoll_ctl(event_loop->epfd, op, fd, &ee) == -1) return -1;

    return 0;
    
}


void epoll_del_event(epoll_struct *event_loop, int fd, int mask)
{
    struct epoll_event ee;
    struct file_event *fe = &event_loop->file_events[fd];

    if (fe->mask == EV_NONE) return;
    fe->mask = fe->mask & (~mask);

    ee.events = 0;
    if (fe->mask & EV_READABLE) ee.events |= EPOLLIN;
    if (fe->mask & EV_WRITABLE) ee.events |= EPOLLOUT;
    ee.data.u64 = 0;
    ee.data.fd = fd;

    if (fe->mask != EV_NONE) {
        epoll_ctl(event_loop->epfd, EPOLL_CTL_MOD, fd, &ee);
    } else {
        epoll_ctl(event_loop->epfd, EPOLL_CTL_DEL, fd, &ee);
    }
}



void epoll_clean(epoll_struct *event_loop, int fd, int linger)
{
    struct epoll_event ee;
    struct file_event *fe = &event_loop->file_events[fd];

    fe->mask = EV_NONE;
    if (fe->data) client_free(fe->data);
    fe->data = NULL;

    epoll_ctl(event_loop->epfd, EPOLL_CTL_DEL, fd, &ee);

    if (linger) set_linger(fd, 0, NULL);
    close(fd);

}




void reply_callback(epoll_struct *event_loop, int fd, void *data)
{
    int size;
    client *c = (client *)data;
    int left = CSTR_LEN(c->sdata) - c->send_pos;

    size = write(fd, c->sdata + c->send_pos, left);
    if (size == -1){
        if (errno == EAGAIN){
            size = 0;       
        }
        else{
            log_debug("Reply error %s", strerror(errno));
            epoll_clean(event_loop, fd, USE_LINGER);
        }
    }

    if (size == left){
        log_debug("Server response: %s", c->sdata);
        cstrclear(c->sdata);
        c->send_pos = 0;
        epoll_del_event(event_loop, fd, EV_WRITABLE);
    }
    else{
        c->send_pos += size;
    }
     
}



void client_callback(epoll_struct *event_loop, int fd, void *data)
{
    int n;
    
    client *c = (client *)data;
    int readbuf = RECVBUF/2; 
    if (CSTR_FREE(c->recvdata) < readbuf){
        c->recvdata = cstrexpand(c->recvdata, readbuf);
    }
        
    n = read(fd, CSTR_FREEBUF(c->recvdata), CSTR_FREE(c->recvdata));
    (CSTR_FREEBUF(c->recvdata))[n] = '\0';
    cstrupdlen(c->recvdata, n);

    if (n == -1) {
        if (errno == EAGAIN){
            n = 0;
        }
        else{
            log_debug("Read error %s, closed fd = %d", strerror(errno), c->fd);
            epoll_clean(event_loop, fd, USE_LINGER);
        }
        return;
    }
    else if (n == 0){
        log_debug("Connect closed fd = %d", c->fd);
        epoll_clean(event_loop, fd, 0);
        return;
    }

    if (packet_parse(c, c->recvdata)){
        log_debug("Error data %s, closed fd = %d", c->recvdata, c->fd);
        epoll_clean(event_loop, fd, USE_LINGER);
        return;
    }


}



void listen_callback(epoll_struct *event_loop, int fd, void *data)
{
    int conn_sock;
	struct sockaddr_in cliaddr;
    socklen_t addrlen;
    client *c;    

    addrlen = sizeof(cliaddr);

    conn_sock = accept(fd, (struct sockaddr *) &cliaddr, &addrlen);

    log_debug("Connect from %s, %d , fd = %d", 
               inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port), conn_sock);


    if (conn_sock == -1) {
      	log_error("accept error");
		return;
    }

    set_nonblock(conn_sock);
    sock_keep_alive(conn_sock);
			
    c = client_new(event_loop, conn_sock);
}



int server_start()
{

    int listen_sock, nfds,  n;

    epoll_struct event_loop;

    signal(SIGPIPE, SIG_IGN);


    if ((listen_sock = tcp_listen(g_config.ip, g_config.port)) == -1) return -1;

    if (epoll_struct_create(&event_loop)) return -1;

    if (epoll_add_event(&event_loop, listen_sock, EV_READABLE, listen_callback, NULL)) return -1;


    while(1){
        nfds = epoll_wait(event_loop.epfd, event_loop.events, MAX_EVENTS, -1);
        if (nfds == -1) {
			/* 便于使用GDB */;
			if (errno != EINTR){
		        log_error("epoll_wait error");
                return -1;
			}
        }

        for (n = 0; n < nfds; ++n) {
			// 处理可读请求 
            
            struct epoll_event *e = &event_loop.events[n];
            struct file_event *fe = &event_loop.file_events[e->data.fd];

		    if (e->events & EPOLLIN) {
                fe->read_callback(&event_loop, e->data.fd, fe->data);
            }
            // 处理可写请求 
			else if (e->events & EPOLLOUT) {
                fe->write_callback(&event_loop, e->data.fd, fe->data);
            }
        } 

	}
    
    return 0;
}
