/***************************
 *文件名称：cstr.c
 *功能描述：字符串处理
 *作    者：LYC
 *创建日期：2013-04-07
 *编码格式：utf-8
 **************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "cstr.h"
#include "define.h"
#include "clist.h"


cstr cstrnew(const char *str)
{
	RTIFNULL(str);
	return cstrnew_len(str, strlen(str));
}



cstr cstrnew_len(const char *str, int len)
{
	cstr_hdr *header;
	int realen = MIN(strlen(str), len);
	int alloclen = ALIGN(len, 16);

	header = malloc(alloclen + sizeof(cstr_hdr) + 1);
	RTIFNULL(header);

	header->len = realen;
	header->free = alloclen - realen;
	memcpy(header->buf, str, realen);	
	header->buf[header->len] = '\0';

	return header->buf;
}



cstr cstrbuf(int len)
/* 申请一个buf */
{
	return cstrnew_len("", len);
}



cstr cstrexpand(cstr str, int len)
/* 若剩余空间小于len，则扩张 */
{
	#define MAX_BLOCK 1024*1024
	if (CSTR_FREE(str) >= len){
		return str;
	}

	int newlen = CSTR_LEN(str) + len;
	if (newlen > MAX_BLOCK){
		newlen += MAX_BLOCK;
	}
	else{
		newlen <<= 1;
	}
		
	cstr_hdr *hdr;
	hdr = realloc(CSTR_HDR(str), newlen + sizeof(cstr_hdr) + 1);
	RTIFNULL(hdr);
	hdr->free = newlen - hdr->len;

	return hdr->buf;
}



void cstrupdlen(cstr str, int len)
/* 使用read或者memset增加了buf长度len时使用，buf为字符串时len可使用0 */
{
	cstr_hdr *hdr = CSTR_HDR(str);

	if (len == 0){
		len = strlen(hdr->buf) - hdr->len;
	}
	
	hdr->free -= len;
	hdr->len += len;
}


void cstrclear(cstr str)
/* 字符串变为空，空间保留 */
{
	cstr_hdr *hdr = CSTR_HDR(str);

	hdr->free += hdr->len;
	hdr->len = 0;
	hdr->buf[0] = '\0';
}


void cstrcut_head(cstr str, int len)
/* 切割字符串头部 */
{
	cstr_hdr *hdr = CSTR_HDR(str);
    if (hdr->len <= len) {
	    hdr->free += hdr->len;
    	hdr->len = 0;
	    hdr->buf[0] = '\0';
        return;
    }

    memmove(str, str+len, (hdr->len-len));
    
    hdr->free += len;
    hdr->len -= len;
    hdr->buf[hdr->len] = 0;
    
}


void cstrcut_tail(cstr str, int len)
{
    cstr_hdr *hdr = CSTR_HDR(str);
    if (hdr->len <= len) {
	    hdr->free += hdr->len;
    	hdr->len = 0;
	    hdr->buf[0] = '\0';
        return;
    }     

    hdr->free += len;
    hdr->len -= len;
    hdr->buf[hdr->len] = 0;
    
}


cstr cstrcat_len(cstr dest, const char *src, int len)
{	
	cstr_hdr *hdr = CSTR_HDR(dest);

	dest = cstrexpand(dest, len);
	RTIFNULL(dest);

	hdr = CSTR_HDR(dest);
	memcpy(dest + hdr->len, src, len);
	hdr->len += len;
	hdr->free -= len;
	dest[hdr->len] = '\0';

	return dest;
				
}



cstr cstrcat(cstr dest, const char *src)
{
	if (dest == NULL || src == NULL){
		return NULL;		
	}

	return cstrcat_len(dest, src, strlen(src));
}


cstr cstrcatvprintf(cstr dest, const char *fmt, va_list ap)
{
	va_list cpy;
    char *buf; 
    size_t buflen;

    while(1) {
        buf = CSTR_FREEBUF(dest);
        buflen = CSTR_FREE(dest);
        buf[buflen-2] = '\0';

        va_copy(cpy,ap);
        vsnprintf(buf, buflen, fmt, cpy);

        if (buf[buflen-2] != '\0') {
            cstrexpand(dest, buflen << 1); 
            continue;
        }   
        break;
    }

    cstrupdlen(dest, 0);
	return dest;

}


cstr cstrcatprintf(cstr dest, const char *fmt, ...)
{
	if (dest == NULL){
		dest = cstrbuf(strlen(fmt) << 1);
	}
	
	va_list ap;
	va_start(ap, fmt);
	dest = cstrcatvprintf(dest, fmt, ap);
	va_end(ap);

	return dest;
}


/* str 字符串 sep 分隔符 count实际分割数, maxcount 最大分割数 */
cstr *cstrsplit(const char *str, char *sep, int *count, int maxcount)
{
	return cstrsplit_len(str, strlen(str), sep, strlen(sep), count, maxcount);
}

cstr *cstrsplit_len(const char *str, int len, char *sep, int seplen, int *count, int maxcount)
{
	clist *list;
	cstr *tokens;
	cstr tmp;
	int b,s,c;

	list = clist_new();
	b = s = 0;
	c = 1;

	while (s <= len-seplen){
		if (str[s] == sep[0] && memcmp(str + s, sep, seplen) == 0){
			tmp = cstrnew_len(str+b, s-b);
			list = clist_append(list, tmp);
			s = s + seplen;
			b = s;
			c++;

			if (maxcount && maxcount == c){
				break;
			}
		}
		else{
			s++;
		}
	}
	
	if (b <= len){
		tmp = cstrnew_len(str+b, len-b);
		list = clist_append(list, tmp);
	}

	if (count){
		*count = c;
	}

	tokens = malloc(sizeof(cstr)*(c+1));

	clist_node *node = list->head;

	for (s = 0; s < c; s++)
	{
		tokens[s] = node->data;
		node = node->next;
	}

	tokens[c] = NULL;

	clist_free(list, NULL);

	return tokens;

}


void cstrfree(cstr str)
{
	free(CSTR_HDR(str));
}


void cstrfree_tokens(cstr *tokens)
{
	int i = 0;
	while(tokens[i])
	{
		cstrfree(tokens[i++]);
	}
	
	free(tokens);
}


