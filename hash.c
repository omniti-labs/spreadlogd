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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "hash.h"

extern int nr_open;
static int myprime[20] = {
        3,5,7,11,13,17,23,31,37,41,43,47,53,59,61,67,71,73,83,87
};

int gethash(void *hostheader, hash_element *hash) {
  int a, i;
  hash_element *elem;
  a = hashpjw(hostheader,nr_open);
  if(hash[a].fd == -1) return -1;  /* return -1 if element is not here */
  for(i=0;i<nr_open; i++) {
    elem = &hash[(a+(i*myprime[a%20]))%nr_open];
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
  int a, i;
  a = hashpjw(b.hostheader,nr_open);
  for(i=0;i<nr_open; i++)
    if((hash[(a+(i*myprime[a%20]))%nr_open].fd) == -1) {
      hash[(a+(i*myprime[a%20]))%nr_open] = b;
      return;
    }
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

