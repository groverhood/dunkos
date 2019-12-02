
#include <kern/vmmgmt.h>
#include <kern/paging.h>

static struct frame *frame_table;

void init_frame_table(void)
{
	size_t user_pages = get_user_pages()
	frame_table = calloc(user_pages, sizeof *frame_table);

	struct frame *f = frame_table;
	while (f != frame_table + user_pages) {
		list_init(&f->aliases);
		f++;
	}
}

struct frame *allocate_frame(void)
{

}

struct frame *evict_frame(void)
{
	
}

void *frame_to_kernaddr(struct frame *f)
{
	uintptr_t upage_number = ((uintptr_t)f - (uintptr_t)frame_table) / sizeof *f;
	return (uint8_t *)get_user_base() + upage_number * PAGESIZE;
}