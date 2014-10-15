#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <stdarg.h>
#include <getopt.h>
#include <signal.h>

#include "chash.h"
#include "define.h"


chashtable *chash_new(chash_func has_func, ccmp_func cmp_func, unsigned long len)
{
    chashtable *hashtable = malloc(sizeof(chashtable));

    hashtable->size = MAX(ALIGN(len, 2), CMIN_HT_LEN);
    hashtable->initsize = hashtable->size;
    hashtable->ht = malloc(sizeof(chash_node *) * hashtable->size);
    memset(hashtable->ht, 0, sizeof(chash_node *) * hashtable->size);
    hashtable->used = 0;

    hashtable->hash_func = has_func;
    hashtable->cmp_func = cmp_func;
    hashtable->free_key = NULL;
    hashtable->free_value = NULL;


    return hashtable;
}



void chash_set_freefunc(chashtable *hashtable, cfree_key free_key,  cfree_value free_value)
{
    hashtable->free_key = free_key;
    hashtable->free_value = free_value;
}


static chash_node *remove_node(chashtable *hashtable, void *key, unsigned long idx)
{
    chash_node *prev = NULL;
    chash_node *list = hashtable->ht[idx];
    while(list){
        if (hashtable->cmp_func(list->key, key) == 0){
            if (prev == NULL) {
                hashtable->ht[idx] = list->next;
            }
            else{
                prev->next = list->next;
            }
            return list;
        }
        prev = list;
        list = list->next;
    }

    return list;

}


void reset_slot(chashtable *hashtable, unsigned long idx)
{
    unsigned long mod = hashtable->size >> 1;
    unsigned long nidx;

    while(mod >= hashtable->initsize){
        idx = idx & (mod - 1);
        if (hashtable->ht[idx]){
            chash_node *prev = NULL;
            chash_node *nextnode, *node = hashtable->ht[idx];
            while(node){
                nidx = HASHINDEX(hashtable, node->key);
                if (nidx != idx){
                    if (prev == NULL) {
                        hashtable->ht[idx] = node->next;
                    }
                    else{
                        prev->next = node->next;
                    }
                    nextnode = node->next;
                    chash_addnode(hashtable, node, nidx);
                    node = nextnode;
                }
                else{
                    prev = node;
                    node = node->next;
                }
            }   
            break;
        } 
        mod >>= 1;
    }
}


inline void chash_addnode(chashtable *hashtable, chash_node *node, unsigned long idx)
{
    node->next = hashtable->ht[idx];
    hashtable->ht[idx] = node;
}


void chash_addnew(chashtable *hashtable, void *key, void *value, unsigned long idx)
{
    chash_node *newnode = malloc(sizeof(chash_node));
    newnode->key = key;
    newnode->value = value;
    newnode->next = hashtable->ht[idx];
    hashtable->ht[idx] = newnode;
}


void chash_expand(chashtable *hashtable)
{
    hashtable->size <<= 1;
    void *tmp;
    tmp = malloc(sizeof(chash_node *) * hashtable->size);
    if (tmp){
        memset(tmp, 0, sizeof(chash_node *) * hashtable->size);
        memcpy(tmp, hashtable->ht, (sizeof(chash_node *) * hashtable->size >> 1));
        free(hashtable->ht);
        hashtable->ht = tmp;
    }
    else{
        hashtable->size >>= 1;
    }
}


void chash_set(chashtable *hashtable, void *key,  void *value)
{
    unsigned long idx;

    if (hashtable->used >= hashtable->size){
        chash_expand(hashtable);
    }

    idx = HASHINDEX(hashtable, key);

    if (hashtable->ht[idx] == NULL && idx >= hashtable->initsize) {
        reset_slot(hashtable, idx);
    }

    chash_node *node = hashtable->ht[idx];

    if (node) {
        do {
            if (hashtable->cmp_func(node->key, key) == 0){
                break;    
            }
            node = node->next;
        }
        while (node);

        if (node){
            if (hashtable->free_key){
                hashtable->free_key(node->key);
                node->key = key;
            }
            if (hashtable->free_value){
                hashtable->free_value(node->value);
            }
            node->value = value;
        }
        else{
            chash_addnew(hashtable, key, value, idx);
        }
    
    }
    else{
        chash_addnew(hashtable, key, value, idx);
    }   

    hashtable->used++;

}



void *chash_get(chashtable *hashtable, void *key)
{
    unsigned long idx = HASHINDEX(hashtable, key);

    if (hashtable->ht[idx] == NULL && idx >= hashtable->initsize) {
        reset_slot(hashtable, idx);
    }

    chash_node *node = hashtable->ht[idx];
    RTIFNULL(node);

    do {
        if (hashtable->cmp_func(node->key, key) == 0){
            break;    
        }
        node = node->next;
    }
    while (node);

    RTIFNULL(node);
    return node->value;

}


void freenode(chashtable *hashtable, chash_node *node)
{


    if (hashtable->free_key){
        hashtable->free_key(node->key);
    }    

    if (hashtable->free_value){
        hashtable->free_value(node->value);
    }    

    free(node);
    
}



int chash_del(chashtable *hashtable, void *key)
{
    unsigned long idx = HASHINDEX(hashtable, key);

    if (hashtable->ht[idx] == NULL && idx >= hashtable->initsize) {
        reset_slot(hashtable, idx);
    }

    chash_node *node = hashtable->ht[idx];
    RTINTIFNULL(node, -1);

    node = remove_node(hashtable, key, idx);
    RTINTIFNULL(node, -1);
    freenode(hashtable, node);

    return 0;
}


void chash_free(chashtable *hashtable)
{
    if (hashtable->ht){
        unsigned long i;
        int count = 0;
        for(i = 0; i < hashtable->size; i++)
        {
            chash_node *nextnode, *node = hashtable->ht[i];
            while (node){
                nextnode = node->next;
                count += 1;
                freenode(hashtable, node);
                node = nextnode;
            }
        }
        free(hashtable->ht);
    }
    free(hashtable);
}


void show_status(chashtable *hashtable)
{
    
    unsigned long count = 0;
    chash_node *node;
    if (hashtable->ht){
        unsigned long i;
        for(i = 0; i < hashtable->size; i++)
        {
            node = hashtable->ht[i];
            if (node){
                count+=1;    
            }
        }
    }

    printf("==========Hash Status==========\n");
    printf("Hashtable size %ld\n", hashtable->size);
    printf("Hashtable initsize %ld\n", hashtable->initsize);
    printf("Hashtable used %ld\n", hashtable->used);
    printf("Hashtable slots %ld\n", count);

}




/************* hash functions ******************/

unsigned long SDBMHash(char *str)  
{  
    unsigned long hash = 0;  
   
    while (*str)  
    {  
        // equivalent to: hash = 65599*hash + (*str++);  
        hash = (*str++) + (hash << 6) + (hash << 16) - hash;  
    }  
   
    return (hash & 0x7FFFFFFF);  
}  
   
// RS Hash Function  
unsigned long RSHash(char *str)  
{  
    unsigned long b = 378551;  
    unsigned long a = 63689;  
    unsigned long hash = 0;  
   
    while (*str)  
    {  
        hash = hash * a + (*str++);  
        a *= b;  
    }  
   
    return (hash & 0x7FFFFFFF);  
}  
   
// JS Hash Function  
unsigned long JSHash(char *str)  
{  
    unsigned long hash = 1315423911;  
   
    while (*str)  
    {  
        hash ^= ((hash << 5) + (*str++) + (hash >> 2));  
    }  
   
    return (hash & 0x7FFFFFFF);  
}  
   
// P. J. Weinberger Hash Function  
unsigned long PJWHash(char *str)  
{  
    unsigned long BitsInUnignedInt = (unsigned long)(sizeof(unsigned long) * 8);  
    unsigned long ThreeQuarters  = (unsigned long)((BitsInUnignedInt  * 3) / 4);  
    unsigned long OneEighth      = (unsigned long)(BitsInUnignedInt / 8);  
    unsigned long HighBits        = (unsigned long)(0xFFFFFFFF) << (BitsInUnignedInt - OneEighth);  
    unsigned long hash            = 0;  
    unsigned long test            = 0;  
   
    while (*str)  
    {  
        hash = (hash << OneEighth) + (*str++);  
        if ((test = hash & HighBits) != 0)  
        {  
            hash = ((hash ^ (test >> ThreeQuarters)) & (~HighBits));  
        }  
    }  
   
    return (hash & 0x7FFFFFFF);  
}  
   
// ELF Hash Function  
unsigned long ELFHash(char *str)  
{  
    unsigned long hash = 0;  
    unsigned long x  = 0;  
   
    while (*str)  
    {  
        hash = (hash << 4) + (*str++);  
        if ((x = hash & 0xF0000000L) != 0)  
        {  
            hash ^= (x >> 24);  
            hash &= ~x;  
        }  
    }  
   
    return (hash & 0x7FFFFFFF);  
}  
   
// BKDR Hash Function  
unsigned long BKDRHash(char *str)  
{  
    unsigned long seed = 131; // 31 131 1313 13131 131313 etc..  
    unsigned long hash = 0;  
   
    while (*str)  
    {  
        hash = hash * seed + (*str++);  
    }  
   
    return (hash & 0x7FFFFFFF);  
}  
   
// DJB Hash Function  
unsigned long DJBHash(char *str)  
{  
    unsigned long hash = 5381;  
   
    while (*str)  
    {  
        hash += (hash << 5) + (*str++);  
    }  
   
    return (hash & 0x7FFFFFFF);  
}  
   
// AP Hash Function  
unsigned long APHash(char *str)  
{  
    unsigned long hash = 0;  
    int i;  
   
    for (i=0; *str; i++)  
    {  
        if ((i & 1) == 0)  
        {  
            hash ^= ((hash << 7) ^ (*str++) ^ (hash >> 3));  
        }  
        else  
        {  
            hash ^= (~((hash << 11) ^ (*str++) ^ (hash >> 5)));  
        }  
    }  
   
    return (hash & 0x7FFFFFFF);  
}  





