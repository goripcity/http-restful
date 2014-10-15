#ifndef __AREDIS_H
#define __AREDIS_H

#include <hiredis/hiredis.h>
#include <hiredis/async.h>

#include "network.h"

void redis_attach(client *client, redisAsyncContext *ac);

#endif /* _AREDIS_H */
