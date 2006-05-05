/* ======================================================================
 * Copyright (c) 2000 George Schlossnagle
 * All rights reserved.
 * The following code was written by George Schlossnagle <george@lethargy.org>
 * This code was written to facilitate clustered logging via Spread,
 * particularly in conjunction with the mod_log_spread module for Apache.
 * More information on Spread can be found at http://www.spread.org/
 * More information on mod_log_spread can be found at 
 * http://www.backhand.org/mod_log_spread/
 * Please refer to the LICENSE file before using this software.
 * ======================================================================
*/

#include "sld_config.h"
#include "hash.h"

int __fhash_size = 1024;
static int myprime[20] = {
        3,5,7,11,13,17,23,31,37,41,43,47,53,59,61,67,71,73,83,87
};

int gethash(void *vhostheader, hash_element *hash) {
  int a, i;
  hash_element *elem;
  char *hostheader = (char *)vhostheader;
  a = hashpjw(hostheader,FHASH_SIZE);
  if(hash[a].fd == -1) {
    return -1;  /* return -1 if element is not here */
  }
  for(i=0;i<FHASH_SIZE; i++) {
    elem = &hash[(a+(i*myprime[a%20]))%FHASH_SIZE];
    /* return -1 if element is not possibly in hsah*/
    if (elem->fd == -1)
      return -1;
    /*return fd if the element matches */
    if(!strcmp(hostheader,elem->hostheader))
      return elem->fd;
  }
  return -1;
}

void inshash(hash_element b, hash_element *hash) {
  int a, i, off;
  a = hashpjw(b.hostheader,FHASH_SIZE);
  if(hash[a].fd == -1) {
    hash[a] = b;
    return;
  }
  for(i=0;i<FHASH_SIZE; i++)
    off = (a+(i*myprime[a%20]))%FHASH_SIZE;
    if((hash[off].fd) == -1) {
      hash[off] = b;
      return;
    }
  close(hash[a].fd);
  hash[a] = b;
}

int hashpjw(const void *key, const int size) {
  const char *ptr;
  unsigned int val = 0;
  ptr = key;
  while (*ptr != '\0') {
    int tmp;
	val = ((val << 1) + ((*ptr)*31 >> 5)) >> 1;
    if ((tmp = (val & 0xf0000000)) != 0) {
      val = val ^ (tmp >> 24);
      val = val ^ tmp;
    }
    ptr++;
  }
  return (int) (val % size);
}

