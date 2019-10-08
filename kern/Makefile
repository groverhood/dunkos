
TARGET = x86_64-elf
OPT_LVL = -O2
LD = ../cross/bin/$(TARGET)-ld
AS = ../cross/bin/$(TARGET)-as
CC = ../cross/bin/$(TARGET)-gcc -c $(OPT_LVL) -mgeneral-regs-only -mno-red-zone
BUILD_DIR = .build
BOOT_DIR = bs

LIB_INCLUDES = -Ilib/include

KERN_SUB_DEPS = mb.o boot.o page_enable.o gdt.o long.o main.o
KERN_SUB_OBJECTS = $(foreach o, $(KERN_SUB_DEPS), $(BUILD_DIR)/$(o))
KERN_OBJECT = $(BUILD_DIR)/kern.o

LIB_CSOURCES = $(shell find ./lib -regex .*\.c -type f -print)
LIB_SSOURCES = $(shell find ./lib -regex .*\.s -type f -print)
LIB_SUB_OBJECTS = $(foreach o, $(LIB_CSOURCES:%.c=%.o) $(LIB_SSOURCES:%.s=%.o), $(BUILD_DIR)/$(notdir $(o)))
LIB_OBJECT = $(BUILD_DIR)/lib.o

all: fresh image clean

fresh:
	rm -vf $(BUILD_DIR)/* && rm -vf $(BOOT_DIR)/iso/boot/kernel.bin && mkdir -p $(BUILD_DIR)

image: kernel
	grub-mkrescue /usr/lib/grub/i386-pc -o $(BUILD_DIR)/kernel.iso $(BOOT_DIR)/iso --verbose

kernel: kern-object lib-object
	$(LD) -n -o $(BOOT_DIR)/iso/boot/kernel.bin -T $(BOOT_DIR)/boot.ld $(KERN_OBJECT) $(LIB_OBJECT)

# =======================
# === BEGIN BOOT MAKE ===
# =======================

kern-object: $(KERN_SUB_DEPS)
	$(LD) -o $(KERN_OBJECT) -r $(KERN_SUB_OBJECTS)

mb.o:
	$(AS) -o $(BUILD_DIR)/mb.o $(BOOT_DIR)/mb.s

boot.o:
	$(AS) -o $(BUILD_DIR)/boot.o $(BOOT_DIR)/boot.s

page_enable.o:
	$(AS) -o $(BUILD_DIR)/page_enable.o $(BOOT_DIR)/page_enable.s

gdt.o:
	$(AS) -o $(BUILD_DIR)/gdt.o $(BOOT_DIR)/gdt.s

long.o:
	$(AS) -o $(BUILD_DIR)/long.o $(BOOT_DIR)/long.s

main.o:
	$(CC) -o $(BUILD_DIR)/main.o $(LIB_INCLUDES) $(BOOT_DIR)/main.c

# =======================
# ===  END BOOT MAKE  ===
# =======================

# =======================
# === BEGIN LIB MAKE  ===
# =======================

lib-object: $(LIB_CSOURCES:%.c=%.o) $(LIB_SSOURCES:%.s=%.o)
	ld -o $(LIB_OBJECT) -r $(LIB_SUB_OBJECTS)
	rm -vf $(LIB_CSOURCES:%.c=%.o)

%.o: %.c
	$(CC) -o $(BUILD_DIR)/$(notdir $@) $(LIB_INCLUDES) $(OPT_LVL) $< 

%.o: %.s
	$(AS) -o $(BUILD_DIR)/$(notdir $@) $<
	

# =======================
# ===  END LIB MAKE   ===
# =======================

clean:
	rm -vf $(BUILD_DIR)/*.o

run: all
	qemu-system-x86_64 -cdrom $(BUILD_DIR)/kernel.iso --curses -d cpu_reset -m 512

run-simple:
	qemu-system-x86_64 -cdrom $(BUILD_DIR)/kernel.iso --curses -d cpu_reset -m 512

run-window:
	qemu-system-x86_64 -cdrom $(BUILD_DIR)/kernel.iso -d cpu_reset -m 512