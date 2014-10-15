#ifndef __CONFIG_H
#define __CONFIG_H

typedef struct config_s {
    short port;         /* server port */
    short rport;        /* redis port */
    char ip[16];        /* server ip */
    char rip[16];       /* redis ip */
} config_t; 

void load_config();

extern config_t g_config;

#endif /* _CONFIG_H */
