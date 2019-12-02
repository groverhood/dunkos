#ifndef DUNKOS_UTIL_HASHTABLE_H
#define DUNKOS_UTIL_HASHTABLE_H

#include <stddef.h>
#include <util/list.h>
#include <stdbool.h>

typedef size_t hash_func(struct list_elem *e);
typedef bool hash_identity(struct list_elem *, struct list_elem *);

struct hashtable {
    size_t buckets, entries;
    struct list *bucket_con;
	hash_func *fn;
	hash_identity *eq;
};

void hashtable_init(struct hashtable *, hash_func *, hash_identity *);

static inline size_t hashtable_get_buckets(struct hashtable *h)
{
	return h->buckets;
}

static inline size_t hashtable_size(struct hashtable *h)
{
	return h->entries;
}

typedef void hash_action(struct list_elem *, void *aux);

void hashtable_destroy(struct hashtable *, hash_action *destructor, void *aux);
struct list_elem *hashtable_find(struct hashtable *, struct list_elem *);
void hashtable_insert(struct hashtable *, struct list_elem *);
void hashtable_foreach(struct hashtable *, hash_action *action, void *aux);

#endif