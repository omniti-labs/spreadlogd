/*
 * Copyright (c) 2001-2006 OmniTI, Inc. All rights reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _ECHASH_H
#define _ECHASH_H

#ifndef API_EXPORT
#define API_EXPORT(a) a
#endif

typedef void (*ECHashFreeFunc)(void *);

typedef struct _ec_hash_bucket {
  const char *k;
  int klen;
  void *data;
  struct _ec_hash_bucket *next;
} ec_hash_bucket;

typedef struct {
  ec_hash_bucket **buckets;
  unsigned int table_size;
  unsigned int initval;
  unsigned int num_used_buckets;
  unsigned int size;
} ec_hash_table;

#define ECHASH_EMPTY { NULL, 0, 0, 0, 0 }

typedef struct {
  void *p2;
  int p1;
} ec_hash_iter;

#define EC_HASH_ITER_ZERO { NULL, 0 }

API_EXPORT(void) echash_init(ec_hash_table *h);
API_EXPORT(int) echash_store(ec_hash_table *h, const char *k, int klen, void *data);
API_EXPORT(int) echash_replace(ec_hash_table *h, const char *k,
                               int klen, void *data,
                               ECHashFreeFunc keyfree, ECHashFreeFunc datafree);
API_EXPORT(int) echash_retrieve(ec_hash_table *h, const char *k, int klen, void **data);
API_EXPORT(int) echash_delete(ec_hash_table *h, const char *k, int klen,
                  ECHashFreeFunc keyfree, ECHashFreeFunc datafree);
API_EXPORT(void) echash_delete_all(ec_hash_table *h, ECHashFreeFunc keyfree,
                       ECHashFreeFunc datafree);
API_EXPORT(void) echash_destroy(ec_hash_table *h, ECHashFreeFunc keyfree,
                    ECHashFreeFunc datafree);

/* This is an iterator and requires the hash to not be written to during the
   iteration process.
   To use:
     ec_hash_iter iter = EC_HASH_ITER_ZERO;

     const char *k;
     int klen;
     void *data;

     while(echash_next(h, &iter, &k, &klen, &data)) {
       .... use k, klen and data ....
     }
*/
API_EXPORT(int) echash_next(ec_hash_table *h, ec_hash_iter *iter,
                            const char **k, int *klen, void **data);
API_EXPORT(int) echash_firstkey(ec_hash_table *h, const char **k, int *klen);
API_EXPORT(int) echash_nextkey(ec_hash_table *h, const char **k, int *klen, char *lk, int lklen);

/* This function serves no real API use sans calculating expected buckets
   for keys (or extending the hash... which is unsupported) */
API_EXPORT(unsigned int) echash__hash(const char *k, unsigned int length,
                                   unsigned int initval);
API_EXPORT(ec_hash_bucket *) echash__new_bucket(const char *k, int klen,
                                                void *data);
API_EXPORT(void) echash__rebucket(ec_hash_table *h, int newsize);
#endif
