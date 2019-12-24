#include <util/hashtable.h>
#include <kern/heap.h>

#define INIT_BUCKETS (10)


void hashtable_init(struct hashtable *h, hash_func *f, hash_identity *id)
{
	h->bucket_con = calloc(INIT_BUCKETS, sizeof *h->bucket_con);

	size_t i;
	for (i = 0; i < INIT_BUCKETS; ++i)
		list_init(h->bucket_con + i);

	h->buckets = INIT_BUCKETS;
	h->entries = 0;

	h->fn = f;
	h->eq = id;
}

void hashtable_destroy(struct hashtable *h, hash_action *destructor, void *aux)
{	
	hashtable_foreach(h, destructor, aux);
	free(h->bucket_con);
}

struct list_elem *hashtable_find(struct hashtable *h, struct list_elem *e)
{
	size_t bucket_index = h->fn(e) % h->buckets;
	struct list *bucket = h->bucket_con + bucket_index;
	struct list_elem *b_el = list_begin(bucket);

	while (b_el != list_end(bucket) && !h->eq(e, b_el)) {
		b_el = list_next(b_el);
	}

	return (b_el == list_end(bucket)) ? NULL : b_el;
}

void hashtable_insert(struct hashtable *h, struct list_elem *e)
{
	size_t bucket_index = h->fn(e) % h->buckets;
	struct list *bucket = h->bucket_con + bucket_index;
	struct list_elem *b_el = list_begin(bucket);

	while (b_el != list_end(bucket) && !h->eq(e, b_el)) {
		b_el = list_next(b_el);
	}

	if (b_el == list_end(bucket)) {
		list_push_back(bucket, e);
		h->entries++;
	} else {
		list_remove(bucket, b_el);
		list_push_back(bucket, e);
	}
}

void hashtable_foreach(struct hashtable *h, hash_action *action, void *aux)
{
	struct list_elem *e;
	size_t b;
	bool no_aux = (aux == NULL);

	for (b = 0; b < h->buckets; ++b) {
		struct list *bucket = h->bucket_con + b;
		if (no_aux)
			aux = bucket;

		for (e = list_begin(bucket); e != list_end(bucket); e = list_next(e))
			action(e, aux);
	}	
}

void hashtable_remove(struct hashtable *h, struct list_elem *e)
{
	size_t bucket_index = h->fn(e) % h->buckets;
	struct list *bucket = h->bucket_con + bucket_index;
	struct list_elem *b_el = list_begin(bucket);

	while (b_el != list_end(bucket) && !h->eq(e, b_el)) {
		b_el = list_next(b_el);
	}

	if (b_el != list_end(bucket)) {
		list_remove(bucket, b_el);
	}
}