#ifndef __DEFINE_H
#define __DEFINE_H

#define ALIGN(x, y) ((x) + (y) - 1) & ~(y - 1)
#define MAX(a, b) (a) > (b) ? a : b
#define MIN(a, b) (a) < (b) ? a : b
#define RTIFNULL(p) if ((p) == NULL) return NULL
#define RTINTIFNULL(p, i) if ((p) == NULL) return i


/* linux kernel 结构体*/

#ifndef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif

#ifndef container_of
#define container_of(ptr, type, member) ({          \
    const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
    (type *)( (char *)__mptr - offsetof(type,member) );})
#endif

/********************/


#endif /* DEFINE_H */
