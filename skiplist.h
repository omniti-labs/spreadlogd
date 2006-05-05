/*
 * Copyright (c) 2000 Theo Schlossnagle <jesus@omniti.com>
 * Copyright (c) 2001-2004 OmniTI, Inc. All rights reserved
 *
 * The following code was written by Theo Schlossnagle for use in the
 * Backhand project at The Center for Networking and Distributed Systems
 * at The Johns Hopkins University.
 *
 */

#ifndef _SKIPLIST_P_H
#define _SKIPLIST_P_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef API_EXPORT
#define API_EXPORT(a) a
#endif

/* This is a skiplist implementation to be used for abstract structures
   within the Spread multicast and group communication toolkit

   This portion written by -- Theo Schlossnagle <jesus@cnds.jhu.eu>
*/

/* This is the function type that must be implemented per object type
   that is used in a skiplist for comparisons to maintain order */
typedef int (*SkiplistComparator) (const void *, const void *);
typedef void (*FreeFunc) (void *);

/*
 This is the function type to use with the builtin traversal functions like level_order_apply
 If it returns 1, the traversal will stop, otherwsie it will continue.
*/
typedef int (*ApplyFunc) (void *);

struct skiplistnode;

typedef struct _iskiplist
{
  SkiplistComparator compare;
  SkiplistComparator comparek;
  unsigned height:8;
  unsigned preheight:8;
  unsigned int size;
  struct skiplistnode *bottom;
  /* These two are needed for appending */
  struct _iskiplist *index;
  struct _iskiplist *agg;
}
Skiplist;

struct skipconn {
  struct skiplistnode *next;
  struct skiplistnode *prev;
};
struct skiplistnode
{
  void *data;
  Skiplist *sl;
  unsigned int height;
  struct skiplistnode *previndex;
  struct skiplistnode *nextindex;
  struct skipconn level[1];
};

#define SLNODESIZE(c) (sizeof(struct skiplistnode) + (c-1)*sizeof(struct skipconn))

/* The height will dictate the size */


API_EXPORT(void) sl_init(Skiplist * sl);
API_EXPORT(void) sl_init_mts(Skiplist * sl);
API_EXPORT(void) sl_set_compare(Skiplist * sl, SkiplistComparator, SkiplistComparator);
API_EXPORT(void) sl_add_index(Skiplist * sl, SkiplistComparator, SkiplistComparator);
API_EXPORT(void) sl_attach_aggregate(Skiplist * sl, Skiplist * agg);
API_EXPORT(struct skiplistnode *) sl_getlist(Skiplist * sl);
API_EXPORT(void *) sl_find_compare(const Skiplist * sl, const void *data,
                      struct skiplistnode **iter, SkiplistComparator func);
API_EXPORT(void *) sl_find_compare_neighbors(const Skiplist * sl, const void *data, struct skiplistnode **iter, struct skiplistnode **prev, struct skiplistnode **next, SkiplistComparator func);
API_EXPORT(void *) sl_find(const Skiplist * sl, const void *data, struct skiplistnode **iter);
API_EXPORT(void *) sl_find_neighbors(const Skiplist * sl, const void *data, struct skiplistnode **iter, struct skiplistnode **prev, struct skiplistnode **next);
API_EXPORT(void *) sl_next(Skiplist * sl, struct skiplistnode **);
API_EXPORT(void *) sl_next(Skiplist * sl, struct skiplistnode **);
API_EXPORT(void *) sl_previous(Skiplist * sl, struct skiplistnode **);

API_EXPORT(struct skiplistnode *) sl_insert_compare(Skiplist * sl,
                                       void *data, SkiplistComparator comp);
API_EXPORT(struct skiplistnode *) sl_insert(Skiplist * sl, void *data);
API_EXPORT(int) sl_remove_compare(Skiplist * sl, void *data,
                      FreeFunc myfree, SkiplistComparator comp);
API_EXPORT(int) sl_remove(Skiplist * sl, void *data, FreeFunc myfree);
API_EXPORT(int) sli_remove(Skiplist * sl, struct skiplistnode *m, FreeFunc myfree);
API_EXPORT(void) sli_remove_all(Skiplist * sl, FreeFunc myfree);
API_EXPORT(void) sl_destruct(Skiplist *sl, FreeFunc myfree);
API_EXPORT(void) sli_destruct_free(Skiplist *sl, FreeFunc myfree);
API_EXPORT(void) sl_node_dataswap(Skiplist *sl, struct skiplistnode *node, void *data);

API_EXPORT(int) sli_find_compare(const Skiplist * sl,
                     const void *data,
                     struct skiplistnode **ret,
                     struct skiplistnode **ret_prev,
                     struct skiplistnode **ret_next,
                     SkiplistComparator comp);
API_EXPORT(void) sl_level_order_apply(Skiplist *sl, ApplyFunc myfunc);

API_EXPORT(struct skiplistnode *) sl_getlist_end(Skiplist *sl);

#define sl_size(a) ((a)->size)

#define SL_index_compare (SkiplistComparator)0x01
#define SL_index_compare_key (SkiplistComparator)0x02
#define SL_INLINE_MAX (SkiplistComparator)0x03

#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif
