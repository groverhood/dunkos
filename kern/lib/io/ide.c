
#include <ide.h>
#include <asm.h>
#include <synch.h>
#include <block.h>
#include <memory.h>
#include <stdbool.h>
#include <interrupt.h>
#include <algo.h>
#include <util/debug.h>

static struct lock ide_lock;
static interrupt_handler dma_handler;
static struct semaphore dma_sema;
static bool expecting_interrupt;

void init_ide(void)
{
	lock_init(&ide_lock);
	semaphore_init(&dma_sema, 0);
	expecting_interrupt = false;
	install_interrupt_handler(INTR_TYPE_DMA, &dma_handler);
}

static void wait_until_ready(void) 
{
  	while ((inb(0x1F7) & 0xC0) != 0x40) {
  		continue;
	}
} 

#define IDE_READ 0x20
#define IDE_WRITE 0x30

void ide_read_sectors(uint32_t start, int sectors, void *dest)
{
}

void ide_write_sectors(uint32_t start, int sectors, void *src)
{
}

static enum interrupt_defer dma_handler(struct interrupt *intr, void *intrframe_, struct register_state *regs)
{

	return INTRDEFR_NONE;
}