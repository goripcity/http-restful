/***************************
 *文件名称：normandy.c
 *功能描述：服务器主程序
 *作    者：LYC
 *创建日期：2014-10-15
 *版    本：SR1
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

#include "normandy.h"


int main(int argc, char **argv)
{

    if (log_init()){
        printf("Init log failed ...\n");
        return -1;
    }

    log_debug("Lanching server ...");

    load_config();

    if (server_start()){
        log_error("Server start failed");
        return -1;
    }
}
