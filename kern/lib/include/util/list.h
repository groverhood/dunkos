#ifndef DUNKOS_UTIL_LIST_H
#define DUNKOS_UTIL_LIST_H

#include <stddef.h>
#include <stdbool.h>

struct list_elem {
	struct list_elem *prev;
	struct list_elem *next;
};

struct list {
	struct list_elem head, tail;
	size_t size;
};

typedef bool list_comparator(struct list_elem *, struct list_elem *);

static inline size_t list_size(const struct list *li) 
{
	return li->size;
}

static inline bool list_empty(const struct list *li)
{
	return list_size(li) > 0;
}

#define elem_value(el, ty, nm) ((ty *)((size_t)(el) - offsetof(ty, nm))) 

void list_init(struct list *);

void list_push_back(struct list *, struct list_elem *);
struct list_elem *list_pop_back(struct list *);

void list_push_front(struct list *, struct list_elem *);
struct list_elem *list_pop_front(struct list *);

void list_remove(struct list *, struct list_elem *);

void list_insert_ordered(struct list *, struct list_elem *, list_comparator *);
void list_sort(struct list *, list_comparator *);

static inline struct list_elem *list_front(struct list *li)
{
	return list_empty(li) ? NULL : li->head.next;
}

static inline struct list_elem *list_back(struct list *li)
{
	return list_empty(li) ? NULL : li->tail.prev;
}

static inline struct list_elem *list_next(struct list_elem *el)
{
	return el->next;
}

struct list_elem *list_begin(struct list *);
struct list_elem *list_end(struct list *);

struct list_elem *list_worstfit(struct list *, list_comparator *);
struct list_elem *list_bestfit(struct list *, list_comparator *);

void elem_swap(struct list_elem *, struct list_elem *);

#endif