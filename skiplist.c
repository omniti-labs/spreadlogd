/*
 * Copyright (c) 2000 Theo Schlossnagle
 * Copyright (c) 2001-2004 OmniTI, Inc. All rights reserved
 *
 * The following code was written by Theo Schlossnagle for use in the
 * Backhand project at The Center for Networking and Distributed Systems
 * at The Johns Hopkins University.
 *
 */

#include "sld_config.h"
#include "skiplist.h"

#define MAXHEIGHT 32

#ifndef MIN
#define MIN(a,b) ((a<b)?(a):(b))
#endif

#ifndef MAX
#define MAX(a,b) ((a>b)?(a):(b))
#endif

static int get_b_rand(void)
{
  static int ph = 32;           /* More bits than we will ever use */
  static unsigned long randseq;
  if (ph > 31) {                /* Num bits in return of lrand48() */
    ph = 0;
    randseq = lrand48();
  }
  ph++;
  return ((randseq & (1 << (ph - 1))) >> (ph - 1));
}

void sli_init(Skiplist * sl)
{
  memset(sl, 0, sizeof(Skiplist));
  sl->compare = (SkiplistComparator) NULL;
  sl->comparek = (SkiplistComparator) NULL;
  sl->height = 0;
  sl->preheight = 0;
  sl->size = 0;
  sl->bottom = NULL;
  sl->index = NULL;
  sl->agg = NULL;
}

#define indexing_comp(a,b) \
  ((((Skiplist *) a)->compare) < (((Skiplist *) b)->compare))?-1: \
    ((((Skiplist *) a)->compare) == (((Skiplist *) b)->compare))?0:1;

#define indexing_compk(a,b) \
  ((SkiplistComparator)(a) < (((Skiplist *) b)->compare))?-1: \
    ((SkiplistComparator)(a) == (((Skiplist *) b)->compare))?0:1;

void sl_init_full(Skiplist * sl)
{
  sli_init(sl);

  sl->index = (Skiplist *) malloc(sizeof(*sl->index));
  assert(sl->index);
  memset(sl->index, 0, sizeof(Skiplist));
  sli_init(sl->index);
  sl_set_compare(sl->index, SL_index_compare, SL_index_compare_key);

  sl->agg = (Skiplist *) malloc(sizeof(*sl->agg));
  assert(sl->agg);
  memset(sl->agg, 0, sizeof(Skiplist));
  sli_init(sl->agg);
  sl_set_compare(sl->agg, SL_index_compare, SL_index_compare_key);
}

void sl_init(Skiplist * sl)
{
  sl_init_full(sl);
}
void sl_set_compare(Skiplist * sl,
                    SkiplistComparator comp, SkiplistComparator compk)
{
  if (sl->compare && sl->comparek) {
    sl_add_index(sl, comp, compk);
  }
  else {
    sl->compare = comp;
    sl->comparek = compk;
  }
}

void sl_add_internal(Skiplist * sl, Skiplist *sli, Skiplist *ni)
{
  struct skiplistnode *m;
  int icount = 0;
#ifdef SLDEBUG
  fprintf(stderr, "Adding index to %p\n", sl);
#endif
  /* Build the new index... This can be expensive! */
  m = sl_insert(sli, ni);
  while (m->level[0].prev)
    m = m->level[0].prev, icount++;
  for (m = sl_getlist(sl); m; sl_next(sl, &m)) {
    int j = icount - 1;
    struct skiplistnode *nsln;
    nsln = sl_insert(ni, m->data);
    /* skip from main index down list */
    while (j > 0)
      m = m->nextindex, j--;
    /* insert this node in the indexlist after m */
    nsln->nextindex = m->nextindex;
    if (m->nextindex)
      m->nextindex->previndex = nsln;
    nsln->previndex = m;
    m->nextindex = nsln;
  }
}
void sl_attach_aggregate(Skiplist *sl, Skiplist *agg)
{
  sl_add_internal(sl, sl->agg, agg);
}
void sl_add_index(Skiplist * sl,
                  SkiplistComparator comp, SkiplistComparator compk)
{
  struct skiplistnode *m;
  Skiplist *ni;
  sl_find(sl->index, (void *) comp, &m);
  if (m)
    return;                     /* Index already there! */
  ni = (Skiplist *) malloc(sizeof(*ni));
  assert(ni);
  memset(ni, 0, sizeof(Skiplist));
  sli_init(ni);
  sl_set_compare(ni, comp, compk);

  sl_add_internal(sl, sl->index, ni);
}

struct skiplistnode *sl_getlist(Skiplist * sl)
{
  if (!sl->bottom)
    return NULL;
  return sl->bottom->level[0].next;
}

void sl_node_dataswap(Skiplist *sl, struct skiplistnode *node, void *data)
{
  while(node->previndex) node = node->previndex;
  while(node) {
    node->data = data;
    node = node->nextindex;
  }
}
void *sl_find(const Skiplist * sl, const void *data, struct skiplistnode **iter)
{
  void *ret;
  struct skiplistnode *aiter;
  if (!sl->compare)
    return 0;
  if (iter)
    ret = sl_find_compare(sl, data, iter, sl->compare);
  else
    ret = sl_find_compare(sl, data, &aiter, sl->compare);
  return ret;
}
void *sl_find_neighbors(const Skiplist * sl, const void *data,
                        struct skiplistnode **iter,
                        struct skiplistnode **prev,
                        struct skiplistnode **next)
{
  struct skiplistnode *aiter, *aprev, *anext;
  if (!sl->compare)
    return 0;
  return sl_find_compare_neighbors(sl, data,
                         iter?iter:&aiter, prev?prev:&aprev, next?next:&anext,
                         sl->compare);
}
void *sl_find_compare(const Skiplist * sli,
                      const void *data,
                      struct skiplistnode **iter, SkiplistComparator comp)
{
  struct skiplistnode *m = NULL;
  struct skiplistnode *i1, *i2;
  const Skiplist *sl;
  if (comp == sli->compare || !sli->index) {
    sl = sli;
  }
  else {
    sl_find(sli->index, (void *) comp, &m);
    assert(m);
    sl = m->data;
  }
  sli_find_compare(sl, data, iter, &i1, &i2, sl->comparek);
  return (*iter) ? ((*iter)->data) : (*iter);
}
void *sl_find_compare_neighbors(const Skiplist * sli,
                      const void *data,
                      struct skiplistnode **iter,
                      struct skiplistnode **prev,
                      struct skiplistnode **next,
                      SkiplistComparator comp)
{
  struct skiplistnode *m = NULL;
  const Skiplist *sl;
  if (comp == sli->compare || !sli->index) {
    sl = sli;
  }
  else {
    sl_find(sli->index, (void *) comp, &m);
    assert(m);
    sl = m->data;
  }
  sli_find_compare(sl, data, iter, prev, next, sl->comparek);
  return (*iter) ? ((*iter)->data) : (*iter);
}
int sli_find_compare(const Skiplist * sl,
                     const void *data,
                     struct skiplistnode **ret,
                     struct skiplistnode **ret_prev,
                     struct skiplistnode **ret_next,
                     SkiplistComparator comp)
{
  struct skiplistnode *m;
  register struct skiplistnode *t;
  register int compared;
  int count = 0;
  int level = 0;
  m = sl->bottom;
  if(m) level = m->height - 1;
  *ret = *ret_prev = *ret_next = NULL;
  while (m && level >= 0) {
    compared = 1;
    if ((t = m->level[level].next) != NULL) {
      if(comp < SL_INLINE_MAX) {
        if(comp == SL_index_compare) {
          compared = indexing_comp(data, t->data);
        } else if(comp == SL_index_compare_key) {
          compared = indexing_compk(data, t->data);
        } else {
          compared = comp(data, t->data);
        }
      } else {
        compared = comp(data, t->data);
      }
    }
    if (compared == 0) {
#ifdef SL_DEBUG
      printf("Looking -- found in %d steps\n", count);
#endif
      m = t;
      *ret = m;
      *ret_prev = (m->level[0].prev && m->level[0].prev != sl->bottom)?m->level[0].prev:NULL;
      *ret_next = m->level[0].next?m->level[0].next:NULL;
      return count;
    }
    if ((t == NULL) || (compared < 0)) {
      if(level == 0) {
        /* mark our left and right... we missed! */
        *ret_prev = (m != sl->bottom)?m:NULL;
        *ret_next = t;
      }
      level--;
      count++;
    } else
      m = t, count++;
  }
#ifdef SL_DEBUG
  printf("Looking -- not found in %d steps\n", count);
#endif
  *ret = NULL;
  return count;
}
void *sl_next(Skiplist * sl, struct skiplistnode **iter)
{
  if (!*iter)
    return NULL;
  *iter = (*iter)->level[0].next;
  return (*iter) ? ((*iter)->data) : NULL;
}
void *sl_previous(Skiplist * sl, struct skiplistnode **iter)
{
  if (!*iter)
    return NULL;
  *iter = (*iter)->level[0].prev;
  if(*iter == sl->bottom) *iter = NULL;
  return (*iter) ? ((*iter)->data) : NULL;
}
struct skiplistnode *sl_insert(Skiplist * sl, void *data)
{
  if (!sl->compare)
    return 0;
  return sl_insert_compare(sl, data, sl->compare);
}

static void sli_insert_multi(Skiplist *slsl, struct skiplistnode *ret) {
  if (slsl != NULL) {
    /* this is a external insertion, we must insert into each index as well */
    struct skiplistnode *p, *ni, *li;
    li = ret;
    for (p = sl_getlist(slsl); p; sl_next(slsl, &p)) {
      ni = sl_insert((Skiplist *) p->data, ret->data);
      assert(ni);
#ifdef SLDEBUG
      fprintf(stderr, "Adding %p to index %p\n", ret->data, p->data);
#endif
      li->nextindex = ni;
      ni->previndex = li;
      li = ni;
    }
  }
}
struct skiplistnode *sl_insert_compare(Skiplist * sl,
                                       void *data, SkiplistComparator comp)
{
  struct skiplistnode *m, *ret, *stack[MAXHEIGHT] = { NULL };
  int level, i, nh = 1, ch;
  int compared = -1;

  ret = NULL;
#ifdef SLDEBUG
  sl_print_struct(sl, "BI: ");
#endif
  if (!sl->bottom) {
    sl->height = 0;
    sl->bottom =
      (struct skiplistnode *) malloc(SLNODESIZE(1));
    assert(sl->bottom);
    memset(sl->bottom, 0, SLNODESIZE(1));
    sl->bottom->height = 0;
    sl->bottom->sl = sl;
  }
  if (sl->preheight) {
    while (nh < sl->preheight && get_b_rand())
      nh++;
  }
  else {
    while (nh <= sl->height && get_b_rand())
      nh++;
  }
  nh = MIN(MAXHEIGHT, nh);      /* Never allow new height to get out of control */

  /* Now we have the new height at which we wish to insert our new node */
  /* Let us make sure that our tree is a least that tall (grow if necessary) */
  if(sl->height == 0) sl->height = sl->bottom->height = 1;
  ch = sl->height;
  if(ch < nh) {
    if(SLNODESIZE(ch) != SLNODESIZE(nh)) {
      m = (struct skiplistnode *) malloc(SLNODESIZE(nh));
      memcpy(m, sl->bottom, SLNODESIZE(ch));
      free(sl->bottom);
      sl->bottom = m;
      for(i=0;i<m->height;i++)
        if(m->level[i].next) m->level[i].next->level[i].prev = m;
    }
    for(i=sl->bottom->height; i<nh; i++) {
      sl->bottom->level[i].prev = sl->bottom->level[i].next = NULL;
    }
  }
  ch = sl->bottom->height = sl->height = MAX(nh,ch);
  level = ch - 1;
  /* Find the node (or node after which we would insert) */
  /* Keep a stack to pop back through for insertion */
  m = sl->bottom;
  while (m && level >= 0) {
    if (m->level[level].next) {
      if(comp < SL_INLINE_MAX) {
        if(comp == SL_index_compare) {
          compared = indexing_comp(data, m->level[level].next->data);
        } else if(comp == SL_index_compare_key) {
          compared = indexing_compk(data, m->level[level].next->data);
        } else {
          compared = comp(data, m->level[level].next->data);
        }
      } else {
        compared = comp(data, m->level[level].next->data);
      }
    }
    if (compared == 0) {
      return 0;
    }
    if ((m->level[level].next == NULL) || (compared < 0)) {
      if (ch <= nh) {
        /* push on stack */
        stack[level] = m;
      }
      level--;
      ch--;
    }
    else {
      m = m->level[level].next;
    }
  }
  /* Pop the stack and insert nodes */
  ret = (struct skiplistnode *) malloc(SLNODESIZE(nh));
  memset(ret, 0, SLNODESIZE(nh));
  ret->height = nh;
  ret->data = data;
  ret->sl = sl;
  for (i=0; i<nh; i++) {
    if(!stack[i]) abort();
    ret->level[i].next = stack[i]->level[i].next;
    stack[i]->level[i].next = ret;
    ret->level[i].prev = stack[i];
    if(ret->level[i].next) ret->level[i].next->level[i].prev = ret;
    ret->nextindex = ret->previndex = NULL;
  }
  sl->size++;

  if (sl->index != NULL)
    sli_insert_multi(sl->index, ret);
  if (sl->agg != NULL)
    sli_insert_multi(sl->agg, ret);
#ifdef SLDEBUG
  sl_print_struct(sl, "AI: ");
#endif
  return ret;
}
struct skiplistnode *sl_append(Skiplist * sl, void *data)
{
  return sl_insert(sl, data);
}

int sl_remove(Skiplist * sl, void *data, FreeFunc myfree)
{
  if (!sl->compare)
    return 0;
  return sl_remove_compare(sl, data, myfree, sl->comparek);
}
int sli_remove_node(Skiplist * sl, struct skiplistnode *m, FreeFunc myfree)
{
  int i;
  struct skiplistnode *p;
  if (!m)
    return 0;
  if (m->sl != sl) {
    fprintf(stderr, "removing element from skiplist it is not in!!!\n");
    *((int *) 0) = 0;
  }
  if (m->nextindex)
    sli_remove_node(m->nextindex->sl, m->nextindex, NULL);
  if (myfree && m->data)
    myfree(m->data);
  sl->size--;
#ifdef SLDEBUG
  sl_print_struct(sl, "BR:");
#endif
  for (i=0; i<m->height; i++) {
    p = m->level[i].prev;
    if(m->level[i].next) m->level[i].next->level[i].prev = p;
    if(p) p->level[i].next = m->level[i].next;
  }
  free(m);
  while (sl->bottom->height &&
         sl->bottom->level[sl->bottom->height-1].next == NULL)
    sl->bottom->height--;
  if(sl->bottom->height == 0) {
    free(sl->bottom);
    sl->bottom = NULL;
    sl->height = 0;
  } else if(sl->height > sl->bottom->height &&
     SLNODESIZE(sl->height) != SLNODESIZE(sl->bottom->height)) {
    p = (struct skiplistnode *) malloc(SLNODESIZE(sl->bottom->height));
    memcpy(p, sl->bottom, SLNODESIZE(sl->bottom->height));
    free(sl->bottom);
    sl->bottom = p;
    for(i=0; i<p->height; i++)
      if(p->level[i].next) p->level[i].next->level[i].prev = p;
    sl->height = sl->bottom->height;
  }
#ifdef SLDEBUG
  sl_print_struct(sl, "AR: ");
#endif
  return MAX(sl->height,1);
}
int sli_remove(Skiplist * sl, struct skiplistnode *m, FreeFunc myfree)
{
  assert(m->sl == sl);
  while(m->previndex) m = m->previndex;
  return sli_remove_node(m->sl, m, myfree);
}
int sl_remove_compare(Skiplist * sli,
                      void *data, FreeFunc myfree, SkiplistComparator comp)
{
  struct skiplistnode *m, *i1, *i2;
  Skiplist *sl;
  if (comp == sli->comparek || !sli->index) {
    sl = sli;
  }
  else {
    sl_find(sli->index, (void *) comp, &m);
    assert(m);
    sl = m->data;
  }
  sli_find_compare(sl, data, &m, &i1, &i2, comp);
  if (!m)
    return 0;
  while (m->previndex)
    m = m->previndex;
  return sli_remove(m->sl, m, myfree);
}

void sli_remove_all(Skiplist * sl, FreeFunc myfree)
{
  /* This must remove even the place holder nodes (bottom though top)
     because we specify in the API that one can free the Skiplist after
     making this call without memory leaks */
  struct skiplistnode *m, *p;
  if(sl->bottom) {
    m = sl->bottom->level[0].next;
    while (m) {
      p = m->level[0].next;
      if (myfree && m && m->data)
        myfree(m->data);
      free(m);
      m = p;
    }
    free(sl->bottom);
  }
  sl->bottom = NULL;
  sl->height = 0;
  sl->size = 0;
}

void sl_destruct(Skiplist *sl, FreeFunc myfree) {
  if(sl->index) {
    sli_remove_all(sl->index, (FreeFunc)sli_destruct_free);
    free(sl->index);
  }
  if(sl->agg) {
    sli_remove_all(sl->agg, NULL);
    free(sl->agg);
  }
  sli_remove_all(sl, myfree);
}
void sli_destruct_free(Skiplist *sl, FreeFunc myfree) {
  sli_remove_all(sl, NULL);
  free(sl);
}

void sl_level_order_apply(Skiplist *sl, ApplyFunc myfunc)
{
  struct skiplistnode *p;
  int level;
  if(sl->size == 0) return;
  for(level = sl->bottom->height-1; level >= 0; level--) {
    p = sl->bottom->level[level].next;
    while(p) {
      if(myfunc && p->height == (level+1))
        if(myfunc(p->data)) return;
      p = p->level[level].next;
    }
  }
}

struct skiplistnode *sl_getlist_end(Skiplist *sl)
{
  /* Q: Why do we do all this work, instead of calling a native
        skiplist function?
     A: We need to get to the _end_ of the list.  By iterating across
        the top level of the skiplist we as far as possible, then drop
        one level and repeat, we simulate a search for a key that is
        greater than the largest in the set.  This is O(lg n).
  */
  struct skiplistnode *p;
  int level;
  if(!sl->bottom) return NULL;
  level = sl->bottom->height-1;
  p = sl->bottom->level[level].next;
  while(level >= 0) {
    while(p->level[level].next) p = p->level[level].next;
    level--;
  }
  if (!p)
    return NULL;
  return p;
}
