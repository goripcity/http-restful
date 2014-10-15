/***************************
 *文件名称：log.c
 *功能描述：
 *作    者：LYC
 *创建日期：2014-10-15
 *版    本：v0.1
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

#include "log.h"


void log_debug(const char *format, ...)
{
    va_list ap;
    va_start(ap,format);
    vfprintf(stdout, format, ap);
    va_end(ap);
}



