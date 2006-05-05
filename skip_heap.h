/*
 * Copyright (c) 2001-2004 OmniTI, Inc. All rights reserved
 *
 * THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF OMNITI
 * The copyright notice above does not evidence any
 * actual or intended publication of such source code.
 *
 * Redistribution of this material is strictly prohibited.
 *
 */

#ifndef _SKIP_HEAP_
#define _SKIP_HEAP_

#include "skiplist.h"

#ifdef __cplusplus
extern "C" {
#endif

static inline void *sl_peek(Skiplist * a)
{
  struct skiplistnode *sln;
  sln = sl_getlist(a);
  if (sln)
    return sln->data;
  return NULL;
}

static inline void *sl_pop(Skiplist * a, FreeFunc myfree)
{
  struct skiplistnode *sln;
  void *data = NULL;
  sln = sl_getlist(a);
  if (sln) {
    data = sln->data;
    sli_remove(a, sln, myfree);
  }
  return data;
}

#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif
