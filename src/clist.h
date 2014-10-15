#ifndef __CLIST_H
#define __CLIST_H

typedef struct clist_node_s {
	struct clist_node_s *prev;
	struct clist_node_s *next;
	void *data;
} clist_node;


typedef struct clist_s {
	clist_node *head;
	clist_node *tail;
	clist_node *empty;		
	int len;
	int free;
} clist;  


#define PREALLOC 8 /* 预申请 */
#define MAXEMPTY 1024 /* 最大队列空闲 */

#define listlen(list) ((list)->len)

clist *clist_new();
clist *clist_append(clist *list, void *data);
clist *clist_prepend(clist *list, void *data);
void clist_remove(clist *list, clist_node *node);
void clist_free(clist *list, void freedata(void *p));

#endif /* _CLIST_H */
