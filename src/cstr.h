#ifndef __CSTR_H
#define __CSTR_H

typedef struct cstr_hdr_s{
	int len;			/* 字符串长度 */
	int free;			/* 剩余空间 */
	char buf[0];
} cstr_hdr;

typedef char *cstr;

#define CSTR_HDR(cstr) (cstr_hdr *)((cstr) - sizeof(cstr_hdr))
#define CSTR_LEN(cstr) ((cstr_hdr *)((cstr) - sizeof(cstr_hdr)))->len
#define CSTR_FREE(cstr) ((cstr_hdr *)((cstr) - sizeof(cstr_hdr)))->free
#define CSTR_FREEBUF(cstr) (cstr + (CSTR_LEN(cstr)))

cstr cstrnew(const char *str);
cstr cstrnew_len(const char *str, int len);
cstr cstrbuf(int len);
cstr cstrexpand(cstr str, int len);
void cstrupdlen(cstr str, int len);
void cstrclear(cstr str);
void cstrcut_head(cstr cstr, int len);
void cstrcut_tail(cstr cstr, int len);
cstr cstrcat(cstr dest, const char *src);
cstr cstrcat_len(cstr dest, const char *src, int len);

cstr cstrcatprintf(cstr dest, const char *fmt, ...);
cstr *cstrsplit(const char *str, char *sep, int *count, int maxcount);
cstr *cstrsplit_len(const char *str, int len, char *sep, int seplen, int *count, int maxcount);

void cstrfree(cstr str);
void cstrfree_tokens(cstr *tokens);

#endif /* _CSTR_H */
