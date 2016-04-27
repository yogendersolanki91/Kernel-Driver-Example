#ifndef __HTAB_H__
#define __HTAB_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "queue.h"

typedef unsigned int u_int;
typedef unsigned int u_int32_t;
typedef unsigned char u_char;
#define inline 

extern const u_int exp2primes[];
int log2c (u_int32_t);

#define nargs1(a) a
#define nargs2(a, b) a, b
#define nargs3(a, b, c) a, b, c
#define mapply(m, a) m a

#define HASH_ENTRY(type)			\
struct {					\
  LIST_ENTRY(type) he_hlink;			\
  TAILQ_ENTRY(type) he_lru;			\
}

#define HASH_TABLE(name, type)			\
struct name {					\
  u_int ht_buckets;				\
  u_int ht_entries;				\
  TAILQ_HEAD(_ht_lru_t, type) ht_lru;			\
  LIST_HEAD(_ht_tab_t, type) *ht_tab;			\
}

#ifdef __cplusplus
#define HASH_INIT(table, buckets, type)				\
do {								\
  u_int __m_i;							\
  (table)->ht_buckets = exp2primes[log2c(buckets)];		\
  (table)->ht_entries = 0;					\
  TAILQ_INIT (&(table)->ht_lru);				\
  xfree ((table)->ht_tab);					\
  (table)->ht_tab = (struct type::_ht_tab_t*) xmalloc (sizeof (*(table)->ht_tab)		\
			     * (table)->ht_buckets);		\
  for (__m_i = 0; __m_i < (table)->ht_buckets; __m_i++) {	\
    LIST_INIT (&(table)->ht_tab[__m_i]);			\
  }								\
} while (0)
#else
#define HASH_INIT(table, buckets)				\
do {								\
  u_int __m_i;							\
  (table)->ht_buckets = exp2primes[log2c(buckets)];		\
  (table)->ht_entries = 0;					\
  TAILQ_INIT (&(table)->ht_lru);				\
  xfree ((table)->ht_tab);					\
  (table)->ht_tab = xmalloc (sizeof (*(table)->ht_tab)		\
			     * (table)->ht_buckets);		\
  for (__m_i = 0; __m_i < (table)->ht_buckets; __m_i++) {	\
    LIST_INIT (&(table)->ht_tab[__m_i]);			\
  }								\
} while (0)
#endif
#define HASH_REMOVE(table, elm, field)			\
do {							\
  LIST_REMOVE ((elm), field.he_hlink);			\
  TAILQ_REMOVE (&(table)->ht_lru, (elm), field.he_lru);	\
  (table)->ht_entries--;				\
} while (0)

#define HASH_TOUCH(table, elm, field)				\
do {								\
  if ((elm)) {							\
    TAILQ_REMOVE (&(table)->ht_lru, (elm), field.he_lru);	\
    TAILQ_INSERT_TAIL (&(table)->ht_lru, (elm), field.he_lru);	\
  }								\
} while (0)

#define HASH_LOOKUP(table, field, nargs, key, hashfn, cmpfn, dest)	\
do {									\
  for ((dest) = (table)->ht_tab[hashfn key				\
			       %(table)->ht_buckets].lh_first;		\
       (dest) && mapply (cmpfn, ((dest), nargs key));			\
       (dest) = (dest)->field.he_hlink.le_next)				\
  ;									\
} while (0)

#define HASH_INSERT(table, field, expand, elm, hashfn)			\
do{									\
  TAILQ_INSERT_TAIL (&(table)->ht_lru, (elm), field.he_lru);		\
  LIST_INSERT_HEAD (&(table)->ht_tab[mapply (hashfn, expand (elm))	\
				    %(table)->ht_buckets],		\
		    (elm), field.he_hlink);				\
  (table)->ht_entries++;						\
} while (0)

#define HASHSEED 5381
static inline u_int
hash(u_int seed, u_char *key, int len)
{
  u_char *end;

  for (end = key + len; key < end; key++)
    seed = ((seed << 5) + seed) ^ *key;
  return seed;
}

#ifdef __cplusplus
}
#endif

#endif
