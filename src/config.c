/***************************
 *文件名称：config.c
 *功能描述：读取配置
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

#include "config.h"
#include "log.h"
#include "cstr.h"

#define CONF "settings.conf"


config_t g_config;  /* global config */

#define LINEBUF 128

void load_config()
{
    FILE *fp;
    char *result;
    char line[LINEBUF];
    cstr *words;

    if ((fp = fopen(CONF, "r")) == NULL){
        log_error("Can't read file "CONF); 
        exit(-1);
    }

    do {
        int count;
        result = fgets(line, LINEBUF, fp);
        if (result == NULL || (result[0] == '#' || result[0] == '\n')) continue;

        words = cstrsplit(result, " ", &count, 10);

        if (strcmp(words[0], "server") == 0) {
            sprintf(g_config.ip, "%s", words[1]);
            g_config.port = (short)atoi(words[2]);
            
        }
        else if (strcmp(words[0], "redis") == 0) {
            sprintf(g_config.rip, "%s", words[1]);
            g_config.rport = (short)atoi(words[2]);
        }
    
        cstrfree_tokens(words);
        
    }
    while(result);

    fclose(fp);
}
