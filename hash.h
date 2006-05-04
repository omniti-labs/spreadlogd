#ifndef _HASH_H_
#define _HASH_H_

typedef struct {
char *hostheader;
int fd;
} hash_element;

#define FHASH_SIZE __fhash_size
extern int __fhash_size;

int gethash(void *, hash_element *);
void inshash(hash_element, hash_element *);
int hashpjw(const void *, const int );

#endif
