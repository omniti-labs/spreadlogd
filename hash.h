#ifndef _HASH_H_
#define _HASH_H_

typedef struct {
char *hostheader;
int fd;
} hash_element;

int gethash(void *, hash_element *);
void inshash(hash_element, hash_element *);
int hashpjw(const void *, const int );

#endif
