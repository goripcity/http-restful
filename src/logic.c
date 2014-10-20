/***************************
 *文件名称：logic.c
 *功能描述：逻辑处理
 *作    者：LYC
 *创建日期：2014-10-16
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

#include "logic.h"
#include "define.h"

#define URIBUF 128;

#define HTTP_RESPONSE  "HTTP/1.1 %d %s\r\nDate: %s\r\nContent-Type: application/json;charset=UTF-8\r\nContent-Length: %d\r\nServer: NormandySR1\r\n\r\n%s"  /* status code, reason, date, lenth, body */


#define ERROR_500 "\"Cannot find data\""
#define ERROR_400 "\"URI format error\""
#define JSON_BODY "{\"result\": %d, \"data\": %s}"
#define JSON_LIST_BODY "{\"result\": %d, \"no\": %d, \"data\": %s}"


void http_response(cstr *buf, int status, const char *reason, const char *date, int lenth, char *body)
{
    if (date == NULL){
        time_t now;
        struct tm * timeinfo;
        char datebuf[64];

        now = time(NULL);
        timeinfo = gmtime(&now);
    
        strftime(datebuf, 64, "%a, %d %b %Y %T GMT", timeinfo);
        date = datebuf;
    }

    cstrcatprintf(*buf, HTTP_RESPONSE, status, reason, date, lenth, body);
}


static void redis_callback(redisAsyncContext *c, void *r, void *privdata) {
    client *client = privdata;
    redisReply *reply = r;
    char body[BUFSIZ];

    if (reply == NULL || (reply->str == NULL && reply->type != 2)) {
        snprintf(body, BUFSIZ, JSON_BODY, 1, ERROR_500);
        http_response(&client->sdata, 500, "ERROR", NULL, strlen(body), body);
        epoll_add_event(client->event_loop, client->fd, EV_WRITABLE, reply_callback, client);
        return;
    }


    if (reply->type == 1){
        log_debug("Redis reply: %s", reply->str);
        snprintf(body, BUFSIZ, JSON_BODY, 0, reply->str);
    }   
    else if (reply->type == 2) { //LIST
        log_debug("Redis reply list, num = %d", reply->elements);
        int i;
        if (reply->elements == 0){
            snprintf(body, BUFSIZ, JSON_LIST_BODY, 0, 0, "[]");
        }
        else{
            cstr buf = cstrnew_len("[", 1024);
            for (i = 0; i < reply->elements - 1; i++) {
                cstrcatprintf(buf, "%s, ", reply->element[i]->str);
            }
            cstrcatprintf(buf, "%s]", reply->element[i++]->str);
        
            snprintf(body, BUFSIZ, JSON_LIST_BODY, 0, i, buf);
            cstrfree(buf);
        }
    }


    http_response(&client->sdata, 200, "OK", NULL, strlen(body), body);
    epoll_add_event(client->event_loop, client->fd, EV_WRITABLE, reply_callback, client);
   
}

cstr get_uri(cstr packet)
{
    char *start = NULL;
    char *end = NULL; 
    char *p = packet + 4;
    for (p; p < packet + CSTR_LEN(packet); p++) {
        if (start == NULL){
            if (*p != ' ') start = p;        
        }
        else{
            if (*p == ' ') {
                end = p;    
                break;
            }
        } 
    }

    if (start == NULL || end == NULL) {
        return NULL;
    }
    
    return cstrnew_len(start, end - start);
    
}



int get_packet(cstr packet, cstr *uri, int *done)
{
    /*
        小端机器字符串对应内存32位数字
        "GET ": "0x20544547"
        "POST": "0x54534F50"
        "PUT ": "0x20545550"
    */

    #define GET  0x20544547
    #define POST 0x54534F50 
    #define PUT  0x20545550


    int status = 0;
    *done = 0;

    uint32_t opt = *(uint32_t *)packet;
    char *split;

    if (opt == GET) {
        split = strstr(packet, "\r\n\r\n");
        if (split == NULL){ 
            goto Done;        
        }
        
        log_debug("GET");
        *uri = get_uri(packet);    
    
        if (split == packet + CSTR_LEN(packet) - 4){ 
            cstrclear(packet);
        }
        else{
            cstrcut_head(packet, split - packet + 4);
            *done = 0;
            return 0;
        }

        goto Done;
    }
    else if (opt == POST) {
        log_debug("POST");
        *uri = get_uri(packet);    
        goto Done; 
    }
    else if (opt == PUT) {
        log_debug("PUT");
        *uri = get_uri(packet);    
        goto Done;
    }
    else{
        status = -1;
        goto Done;
    }
    

Done:
    *done = 1;    
    return status;

    
}



int redis_command(client *c, cstr uri)
{
    char *p, *s;
    int start = 0;
    p = strstr(uri, "?");
    if (p == NULL) { 
        redisAsyncCommand(c->context, redis_callback, c, "GET %b", uri, CSTR_LEN(uri));
        return 0;
    }

    s = strstr(p, "start");
    if (s) {
        start = atoi(s+6);
        start--;
        start = MAX(start, 0);
        *p = '\0';
        redisAsyncCommand(c->context, redis_callback, c, "LRANGE %s %d -1", uri, start);
        return 0;
    }
    else{
        return -1;
    }


}


int packet_parse(client *c, cstr data)
{
    log_debug("Get packet - [fd = %d] : %s", c->fd, data);

    int done, status;
    char body[BUFSIZ];
    cstr result = NULL;

    do {
        status = get_packet(data, &result, &done);
        if (status) return -1;
        if (result) {
            if (c->context == NULL) {
                snprintf(body, BUFSIZ, JSON_BODY, 1, ERROR_500);
                http_response(&c->sdata, 500, "ERROR", NULL, strlen(body), body);
                epoll_add_event(c->event_loop, c->fd, EV_WRITABLE, reply_callback, c);
                cstrfree(result);
                return 0;
            }
            log_debug("result: [%s]", result);
            if (redis_command(c, result)){
                snprintf(body, BUFSIZ, JSON_BODY, 1, ERROR_400);
                http_response(&c->sdata, 400, "ERROR", NULL, strlen(body), body);
                epoll_add_event(c->event_loop, c->fd, EV_WRITABLE, reply_callback, c);
                cstrfree(result);
                return 0;
            }
            cstrfree(result);
        }
    } while(!done);
        
    return 0;

    
}

