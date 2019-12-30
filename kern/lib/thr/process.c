
#include <process.h>
#include <thread.h>
#include <heap.h>
#include <pml4.h>
#include <asm.h>
#include <vmmgmt.h>
#include <filesys.h>
#include <fdtable.h>
#include <util/debug.h>
#include <util/bitmap.h>
#include <string.h>

#define FDCOUNT (128)
#define SEMTABLE_SIZE (128)

void process_init(struct process *pr)
{
    /* Same as &pr->base. */
    struct thread *thr = (struct thread *)pr;
    thread_init(thr);
    thr->magic = PROCESS_MAGIK;
    pr->exit_code = -1;
    page_table_create(&pr->spt);
    pr->fdtable = create_fdtable(pr->cwd, FDCOUNT);
    pr->semtable = kcalloc(SEMTABLE_SIZE, sizeof *pr->semtable);
    pr->semtable_map = bitmap_create(SEMTABLE_SIZE);
    lock_init(&pr->semtable_lock);
}

struct process *current_process(void)
{
    struct thread *cur = get_current_thread();
    return is_process(cur) ? (struct process *)(cur) : cur->parent;
}

struct process *fork_process(void)
{
    struct process *cur = current_process();
    struct process *pr = kcalloc(1, sizeof *pr);
    
    process_init(pr);
    struct thread *thr = process_get_base(pr);

    /* Copy the stack pointer and instruction pointer so as to resume
       execution after the fork() system call. */
    memcpy(&thr->context, &cur->base.context, sizeof(struct thread_context));
    list_push_back(&cur->children, &pr->base.child_elem);

    /* Ensure an identical address space that doesn't mutate the parent's. */
    page_table_copy(pr->spt, cur->spt);

    return pr;
}

static void load_segment(void *start, struct file *source, off_t ofs, 
                         off_t size, bool readonly)
{
    uint8_t *page = start;
    while (size > 0) {
        page_defer_load(page, source, ofs, readonly);

        ofs += PAGESIZE;
        page += PAGESIZE;
        size -= PAGESIZE;
    }
}

#define ELF_MAGIK (*(uint32_t *)((char[]){ 0x7F, 'E', 'L', 'F' }))

struct elf_header {
    uint32_t magic;
    uint8_t class;
    uint8_t endianness;
    uint8_t version;
    uint8_t osabi;
    uint8_t abiversion;
    uint8_t pad[7];
    uint16_t type;
    uint16_t machine;
    uint32_t elfversion;
    void *entrypoint;
    off_t progheader;
    off_t sectheader;
    uint32_t flags;
    uint16_t size;
    uint16_t phentsize;
    uint16_t phentcnt;
    uint16_t shentsize;
    uint16_t shentcnt;
    uint16_t shnameidx;
} __attribute__((packed));

struct program_header {
    uint32_t type;
    uint32_t flags;
    off_t offset;
    void *vaddr;
    void *paddr;
    off_t size;
    off_t memsz;
    uint64_t align;
} __attribute__((packed));

void exec_process(const char *file, char **argv)
{
    assert(sizeof(struct elf_header) == 0x40);
    struct process *cur = current_process();
    struct file *executable = open_file(cur->cwd, file);

    cur->executable = executable;

    struct elf_header header;
    file_read_at(executable, &header, 0, sizeof header);

    assert(header.magic == ELF_MAGIK);
    
    if (header.phentcnt > 0) {
        struct program_header *pht = kcalloc(header.phentcnt, sizeof *pht);
        file_read_at(executable, pht, header.progheader, 
                     header.phentcnt * header.phentsize);

        off_t pht_index;
        for (pht_index = 0; pht_index < header.phentcnt; ++pht_index) {
            struct program_header *phte = (pht + pht_index);
            if (phte->type == 0x1) {
                load_segment(phte->vaddr, executable, phte->offset, 
                             phte->size, !!(phte->flags & FILEINFO_MODE_R));
            }
        }

        kfree(pht);
    }
}

__attribute__((noreturn)) void exit_process(int status)
{
    struct process *cur = current_process();

    cur->exit_code = status;
    
    struct list_elem *el;
    for (el = list_begin(&cur->children); el != list_end(&cur->children);
        el = list_next(el))
    {
        struct thread *chld = elem_value(el, struct thread, child_elem);
        if (is_process(chld))
            semaphore_inc(&((struct process *)chld)->reap_sema);
    }

    page_table_destroy(cur->spt);

    semaphore_inc(&cur->wait_sema);
    semaphore_dec(&cur->reap_sema);

    exit_thread();
    unreachable();
}
