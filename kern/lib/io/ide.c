
#include <ide.h>
#include <asm.h>
#include <synch.h>
#include <block.h>
#include <memory.h>
#include <stdbool.h>
#include <interrupt.h>
#include <algo.h>

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
	barrier();
	size_t sectors_to_read = (sectors & 0xFF);
	wait_until_ready();
	lock_acquire(&ide_lock);
	outb((uint8_t)sectors_to_read, 0x1F2);
	outb(getbyte(start, 0), 0x1F3);
	outb(getbyte(start, 1), 0x1F4);
	outb(getbyte(start, 2), 0x1F5);
	outb(getbyte(start, 3), 0x1F6);
	expecting_interrupt = true;
	outb(IDE_READ, 0x1F7);
	semaphore_dec(&dma_sema);
	insb(0x1F0, dest, sectors_to_read * BLOCK_SECTOR_SIZE);
	lock_release(&ide_lock);
}

void ide_write_sectors(uint32_t start, int sectors, void *src)
{
	barrier();
	size_t sectors_to_write = (sectors & 0xFF);
	wait_until_ready();
	lock_acquire(&ide_lock);
	outb((uint8_t)sectors_to_write, 0x1F2);
	outb(getbyte(start, 0), 0x1F3);
	outb(getbyte(start, 1), 0x1F4);
	outb(getbyte(start, 2), 0x1F5);
	outb(getbyte(start, 3), 0x1F6);
	expecting_interrupt = true;
	outb(IDE_READ, 0x1F7);
	outsb(src, 0x1F0, sectors_to_write * BLOCK_SECTOR_SIZE);
	semaphore_dec(&dma_sema);
	lock_release(&ide_lock);
}

static enum interrupt_defer dma_handler(struct interrupt *intr, void *intrframe_, struct register_state *regs)
{
	barrier();
	if (expecting_interrupt) {
		inb(0x1F7);
		expecting_interrupt = false;
		semaphore_inc(&dma_sema);
	} else puts("Unexpected DMA interrupt.");

	return INTRDEFR_NONE;
}