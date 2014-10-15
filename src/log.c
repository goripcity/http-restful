/***************************
 *文件名称：log.c
 *功能描述：日志
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

#include "log.h"


#define TIMEBUF 64

/*
    30: 黑 31: 红 32: 绿  33: 黄 34: 蓝  35: 紫  36: 深绿 37: 白色 
    none = "\033[0m"
    black = "\033[0;30m"
    dark_gray = "\033[1;30m"
    
    颜色函数
*/

#define USE_COLOR_TERM
//#undef USE_COLOR_TERM

#define NONE "\033[0m"
#define GREEN "\033[0;32m"
#define RED "\033[0;31m"



int log_init()
{
    /* init log file */
    return 0; 
}


static void prefix_time(FILE *stream, const char *level)
{
    struct timeval tv;
    struct tm * timeinfo;
    
    char buf[TIMEBUF];

    gettimeofday(&tv, NULL);
    time(&tv.tv_sec);
    timeinfo = localtime(&tv.tv_sec);
    
    strftime(buf, TIMEBUF, "%F %T", timeinfo);

    fprintf(stream, "%s,%06ld - [%s] - ", buf, tv.tv_usec, level);
    
}



void log_debug(const char *format, ...)
{
    FILE *stream = stdout;

    #ifdef USE_COLOR_TERM
        fprintf(stdout, GREEN);
    #endif
   
    prefix_time(stream, "DEBUG");

    va_list ap;
    va_start(ap,format);
    vfprintf(stream, format, ap);
    va_end(ap);
    
    fprintf(stream, "\n");
    #ifdef USE_COLOR_TERM
        fprintf(stdout, NONE);
        fflush(stdout);
    #endif
    
}


void log_error(const char *format, ...)
{
    FILE *stream = stderr;

    #ifdef USE_COLOR_TERM
        fprintf(stderr, RED);
    #endif
   
    prefix_time(stream, "ERROR");

    va_list ap;
    va_start(ap,format);
    vfprintf(stream, format, ap);
    va_end(ap);
    
    fprintf(stream, "\n");
    #ifdef USE_COLOR_TERM
        fprintf(stderr, NONE);
        fflush(stderr);
    #endif
    
}

