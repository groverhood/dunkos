#include <util/list.h>

void list_init(struct list *li)
{
	struct list_elem *he = &li->head;
	struct list_elem *tl = &li->tail;

	he->prev = NULL;
	he->next = tl;

	tl->prev = he;
	tl->next = NULL;

	li->size = 0;
}

void list_push_back(struct list *li, struct list_elem *el)
{
	struct list_elem *tl = &li->tail;

	if (list_empty(li)) {
		struct list_elem *he = &li->head;
		el->prev = he;
		he->next = el;
	} else {
		struct list_elem *tp = tl->prev;
		el->prev = tp;
		tp->next = el;
	}
	
	tl->prev = el;
	el->next = tl;

	li->size++;
}

struct list_elem *list_pop_back(struct list *li)
{
	struct list_elem *pb = NULL;

	if (!list_empty(li)) {
		pb = list_end(li)->prev;

		struct list_elem *ne = pb->next;
		struct list_elem *pr = pb->prev;

		pr->next = ne;
		ne->prev = pr;

		li->size--;
	}

	return pb;
}

void list_push_front(struct list *li, struct list_elem *el)
{
	struct list_elem *he = &li->head;

	if (list_empty(li)) {
		struct list_elem *tl = &li->tail;
		el->next = tl;
		tl->prev = el;
	} else {
		struct list_elem *hn = he->next;
		el->next = hn;
		hn->prev = el;
	}
	
	he->next = el;
	el->prev = he;

	li->size++;
}

struct list_elem *list_pop_front(struct list *li)
{
	struct list_elem *pf = NULL;

	if (!list_empty(li)) {
		pf = list_begin(li);

		struct list_elem *ne = pf->next;
		struct list_elem *pr = pf->prev;

		pr->next = ne;
		ne->prev = pr;

		li->size--;
	}

	return pf;
}

void list_remove(struct list *li, struct list_elem *el)
{
	if (!list_empty(li)) {
		struct list_elem *pr = el->prev;
		struct list_elem *ne = el->next;

		pr->next = ne;
		ne->prev = pr;

		li->size--;
	}
}

void list_insert_ordered(struct list *li, struct list_elem *el, list_comparator *cmp)
{
	struct list_elem *pr = list_begin(li);

	if (pr) {
		while (cmp(pr, el)) {
			pr = pr->next;
		}
		
		struct list_elem *ne = pr->next;
		pr->next = el;
		el->next = ne;
		el->prev = pr;

	} else {
		li->head.next = el;
		el->prev = &li->head;
		li->tail.prev = el;
		el->next = &li->tail;
	}

	li->size++;
}

void list_sort(struct list *li, list_comparator *cmp)
{

}

struct list_elem *list_begin(struct list *li)
{
	return list_empty(li) ? &li->tail : li->head.next;
}

struct list_elem *list_end(struct list *li)
{
	return &li->tail;
}

struct list_elem *list_worstfit(struct list *li, list_comparator *cmp)
{
	struct list_elem *el;
	struct list_elem *worst = list_end(li);

	for (el = list_begin(li); el != list_end(li); el = list_next(el)) {
		if (!cmp(el, worst)) {
			worst = el;
		}
	}

	return worst;
}

struct list_elem *list_bestfit(struct list *li, list_comparator *cmp)
{
	struct list_elem *el;
	struct list_elem *best = list_end(li);

	for (el = list_begin(li); el != list_end(li); el = list_next(el)) {
		if (cmp(el, best)) {
			best = el;
		}
	}

	return best;
}

void elem_swap(struct list_elem *lf, struct list_elem *ri) 
{
	struct list_elem *lpr = lf->prev;
	struct list_elem *rpr = ri->prev;
	struct list_elem *rne = ri->next;
	struct list_elem *lne = lf->next;

	lpr->next = ri;
	rpr->next = lf;

	lf->prev = rpr;
	ri->prev = lpr;

	lne->prev = ri;
	rne->prev = lf;

	lf->next = rne;
	ri->next = lne;
}