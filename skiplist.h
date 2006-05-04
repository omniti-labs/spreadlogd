/* ======================================================================
 * Copyright (c) 2000 Theo Schlossnagle
 * All rights reserved.
 * The following code was written by Theo Schlossnagle for use in the
 * Backhand project at The Center for Networking and Distributed Systems
 * at The Johns Hopkins University.
 *
 * This is a skiplist implementation to be used for abstract structures
 * and is release under the LGPL license version 2.1 or later.  A copy
 * of this license can be found at http://www.gnu.org/copyleft/lesser.html
 * ======================================================================
*/

#ifndef _SKIPLIST_P_H
#define _SKIPLIST_P_H

/* This is a skiplist implementation to be used for abstract structures
   within the Spread multicast and group communication toolkit

   This portion written by -- Theo Schlossnagle <jesus@cnds.jhu.eu>
*/

/* This is the function type that must be implemented per object type
   that is used in a skiplist for comparisons to maintain order */
typedef int (*SkiplistComparator)(void *, void *);
typedef void (*FreeFunc)(void *);

struct skiplistnode;

typedef struct _iskiplist {
  SkiplistComparator compare;
  SkiplistComparator comparek;
  int height;
  int preheight;
  int size;
  struct skiplistnode *top;
  struct skiplistnode *bottom;
  /* These two are needed for appending */
  struct skiplistnode *topend;
  struct skiplistnode *bottomend;
  struct _iskiplist *index;
} Skiplist;

struct skiplistnode {
  void *data;
  struct skiplistnode *next;
  struct skiplistnode *prev;
  struct skiplistnode *down;
  struct skiplistnode *up;
  struct skiplistnode *previndex;
  struct skiplistnode *nextindex;
  Skiplist *sl;
};


void sl_init(Skiplist *sl);
void sl_set_compare(Skiplist *sl, SkiplistComparator,
		    SkiplistComparator);
void sl_add_index(Skiplist *sl, SkiplistComparator,
		  SkiplistComparator);
struct skiplistnode *sl_getlist(Skiplist *sl);
void *sl_find_compare(Skiplist *sl, void *data, struct skiplistnode **iter,
		      SkiplistComparator func);
void *sl_find_compare_neighbors(Skiplist *sl, void *data, struct skiplistnode **iter, struct skiplistnode **left, struct skiplistnode **right,
		                SkiplistComparator func);
void *sl_find(Skiplist *sl, void *data, struct skiplistnode **iter);
void *sl_find_neighbors(Skiplist *sl, void *data, struct skiplistnode **iter, struct skiplistnode **left, struct skiplistnode ** right);
void *sl_next(Skiplist *sl, struct skiplistnode **);
void *sl_previous(Skiplist *sl, struct skiplistnode **);

struct skiplistnode *sl_insert_compare(Skiplist *sl,
				       void *data, SkiplistComparator comp);
struct skiplistnode *sl_insert(Skiplist *sl, void *data);
int sl_remove_compare(Skiplist *sl, void *data,
		      FreeFunc myfree, SkiplistComparator comp);
int sl_remove(Skiplist *sl, void *data, FreeFunc myfree);
int sli_remove(Skiplist *sl, struct skiplistnode *m, FreeFunc myfree);
void sl_remove_all(Skiplist *sl, FreeFunc myfree);

int sli_find_compare(Skiplist *sl,
		    void *data,
		    struct skiplistnode **ret,
		    SkiplistComparator comp);
int sli_find_compare_neighbors(Skiplist *sl,
		    void *data,
		    struct skiplistnode **ret,
		    struct skiplistnode **left,
		    struct skiplistnode **right,
		    SkiplistComparator comp);

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

#endif
