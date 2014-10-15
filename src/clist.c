/***************************
 *文件名称：clist.c
 *功能描述：双向链表
 *作    者：LYC
 *创建日期：2013-04-07
 *编码格式：utf-8
 **************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "clist.h"
#include "define.h"


static void clist_expand(clist *list, int count)
{
	int i;
	clist_node *node;

	for(i = 0; i < count; i++)
	{
		node = malloc(sizeof(clist_node));
		memset(node, 0, sizeof(clist_node));	
		node->next = list->empty;
		list->empty = node;
		list->free++; 	
	}
	
}


static clist_node *clist_emptypop(clist *list)
{
	clist_node *node;

	list->free--; 
	node = list->empty;
	list->empty = node->next;

	return node;
}



static void clist_emptypush(clist *list, clist_node *node)
{
	if (list->free >= MAXEMPTY){
		free(node);
		return;
	}

	node->next = list->empty;
	list->empty = node;
	list->free++;

}



clist *clist_new()
{
	clist *list = malloc(sizeof(clist));
	RTIFNULL(list);
	memset(list, 0, sizeof(clist));

	clist_expand(list, PREALLOC);

	return list;
}


clist *clist_append(clist *list, void *data)
{
	clist_node *node;

	if (list->free == 0){
		clist_expand(list, MIN(list->len, MAXEMPTY));
	}

	node = clist_emptypop(list);
	node->data = data;

	if (list->tail == NULL){
		list->head = node;
		list->tail = node;
		node->prev = NULL;
		node->next = NULL;
	}
	else{
		list->tail->next = node;
		node->prev = list->tail;
		node->next = NULL;
		list->tail = node;  
	}

	list->len++;
	return list;
}



clist *clist_prepend(clist *list, void *data)
{
	clist_node *node;

	if (list->free == 0){
		clist_expand(list, MIN(list->len, MAXEMPTY));
	}

	node = clist_emptypop(list);
	node->data = data;

	if (list->head == NULL){
		list->head = node;
		list->tail = node;
		node->prev = NULL;
		node->next = NULL;
	}
	else{
		list->head->prev = node;
		node->prev = NULL;
		node->next = list->head;
		list->head = node;
	}

	list->len++;

	return list;
}


void clist_remove(clist *list, clist_node *node)
{
	if (node->prev){
		node->prev->next = node->next;
	}
	else{
		list->head = node->next;
	}

	if (node->next){
		node->next->prev = node->prev;
	}
	else{
		list->tail = node->prev;
	}

	clist_emptypush(list, node);

	list->len--;
		

}



void clist_free(clist *list, void freedata(void *p))
{
	clist_node *node, *p;
	node = list->head;

	while(node){
		if (freedata){
			freedata(node->data);
		}
		p = node;
		node = node->next;
		free(p);
	}

	node = list->empty;
	while(node){
		p = node;
		node = node->next;
		free(p);
	}

	free(list);
	
}
