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

#include "hash.h"

static int myprime[20] = {
        3,5,7,11,13,17,23,31,37,41,43,47,53,59,61,67,71,73,83,87
};

int gethash(void *hostheader, hash_element *hash) {
  int a, i;
  hash_element *elem;
  a = hashpjw(hostheader,NR_OPEN);
  if(hash[a].fd == -1) return -1;  /* return -1 if element is not here */
  for(i=0;i<NR_OPEN; i++) {
    elem = &hash[(a+(i*myprime[a%20]))%NR_OPEN];
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
  a = hashpjw(b.hostheader,NR_OPEN);
  for(i=0;i<NR_OPEN; i++)
    if((hash[(a+(i*myprime[a%20]))%NR_OPEN].fd) == -1) {
      hash[(a+(i*myprime[a%20]))%NR_OPEN] = b;
      return;
    }
}

int hashpjw(const void *key, const int size) {
  const char *ptr;
  unsigned int val = 0;
  ptr = key;
  while (*ptr != '\0') {
    int tmp;
    val = (val << 4) + (*ptr);
    if ((tmp = (val & 0xf0000000)) != 0) {
      val = val ^ (tmp >> 24);
      val = val ^ tmp;
    }
    ptr++;
  }
  return (int) (val % size);
}

