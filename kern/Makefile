
TARGET = x86_64-elf
OPT_LVL = -O2
CC_FLAGS = -c $(OPT_LVL) -mgeneral-regs-only -mno-red-zone -D_DEBUG -D__DUNKOS
LD = ../cross/bin/$(TARGET)-ld
AS = ../cross/bin/$(TARGET)-as
CC = ../cross/bin/$(TARGET)-gcc $(CC_FLAGS)
BUILD_DIR = .build
BOOT_DIR = bs
DISK_SIZE = 64M

LIB_INCLUDES = -Ilib/include/kern -I/lib/include/shared

KERN_SUB_DEPS = mb.o boot.o gdt.o long.o main.o ia32page.o
KERN_SUB_OBJECTS = $(foreach o, $(KERN_SUB_DEPS), $(BUILD_DIR)/$(o))
KERN_OBJECT = $(BUILD_DIR)/kern.o

LIB_CSOURCES = $(shell find ./lib -regex '.*\.c' -not -path '*/usr/*' -type f -print)
LIB_SSOURCES = $(shell find ./lib -regex '.*\.s' -not -path '*/usr/*' -type f -print)
LIB_SUB_OBJECTS = $(foreach o, $(LIB_CSOURCES:%.c=%.o) $(LIB_SSOURCES:%.s=%.o), $(BUILD_DIR)/$(notdir $(o)))
LIB_OBJECT = $(BUILD_DIR)/lib.o

all: fresh image disk clean

fresh:
	rm -vf $(BUILD_DIR)/* && rm -vf $(BOOT_DIR)/iso/boot/kernel.bin && mkdir -p $(BUILD_DIR)

image: kernel
	grub-mkrescue /usr/lib/grub/i386-pc -o $(BUILD_DIR)/kernel.iso $(BOOT_DIR)/iso

kernel: kern-object lib-object
	$(LD) -n -o $(BOOT_DIR)/iso/boot/kernel.bin -T $(BOOT_DIR)/boot.ld $(KERN_OBJECT) $(LIB_OBJECT)

disk:
	qemu-img create -f qcow2 $(BUILD_DIR)/dunkos.img $(DISK_SIZE)

# =======================
# === BEGIN BOOT MAKE ===
# =======================

kern-object: $(KERN_SUB_DEPS)
	$(LD) -o $(KERN_OBJECT) -r $(KERN_SUB_OBJECTS)

mb.o:
	$(AS) -o $(BUILD_DIR)/mb.o $(BOOT_DIR)/mb.s

boot.o:
	$(AS) -o $(BUILD_DIR)/boot.o $(BOOT_DIR)/boot.s

gdt.o:
	$(AS) -o $(BUILD_DIR)/gdt.o $(BOOT_DIR)/gdt.s

long.o:
	$(AS) -o $(BUILD_DIR)/long.o $(BOOT_DIR)/long.s

main.o:
	$(CC) -o $(BUILD_DIR)/main.o $(LIB_INCLUDES) $(BOOT_DIR)/main.c

ia32page.o:
	$(AS) -o $(BUILD_DIR)/ia32page.o $(BOOT_DIR)/ia32page.s

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
	qemu-system-x86_64 -hda $(BUILD_DIR)/dunkos.img -cdrom $(BUILD_DIR)/kernel.iso --curses -m 512