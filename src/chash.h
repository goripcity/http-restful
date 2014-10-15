#ifndef __CHASH_H
#define __CHASH_H

#define CMIN_HT_LEN 8
#define HASHINDEX(h, k) (h)->hash_func((k)) & ((h)->size - 1); 

typedef unsigned long (*chash_func)(void *key);
typedef int (*ccmp_func)(void *k1, void *k2);
typedef int (*cfree_key)(void *key);
typedef int (*cfree_value)(void *value);


typedef struct chash_node_s {
    void *key;                      
    void *value;
    struct chash_node_s *next;    
} chash_node;


typedef struct chashtable_s {
    chash_node **ht;

    chash_func hash_func;
    ccmp_func cmp_func;
    cfree_key free_key;
    cfree_value free_value;
    
    unsigned long size;
    unsigned long initsize;
    unsigned long used;

} chashtable;


chashtable *chash_new(chash_func has_func, ccmp_func cmp_func, unsigned long len);
void chash_set_freefunc(chashtable *hashtable, cfree_key free_key,  cfree_value free_value);
void reset_slot(chashtable *hashtable, unsigned long idx);
inline void chash_addnode(chashtable *hashtable, chash_node *node, unsigned long idx);
void chash_addnew(chashtable *hashtable, void *key, void *value, unsigned long idx);
void chash_set(chashtable *hashtable, void *key, void *value);
void *chash_get(chashtable *hashtable, void *key);
void freenode(chashtable *hashtable, chash_node *node);
int chash_del(chashtable *hashtable, void *key);
void chash_free(chashtable *hashtable);
void show_status(chashtable *hashtable);

/************* hash functions ******************/

unsigned long SDBMHash(char *str);  
unsigned long RSHash(char *str); 
unsigned long JSHash(char *str);  
unsigned long PJWHash(char *str);  /* bad, test result if use it */
unsigned long ELFHash(char *str);  /* not good*/
unsigned long BKDRHash(char *str);  
unsigned long DJBHash(char *str); 
unsigned long APHash(char *str);  

#endif /* _CHASH_H */
