LD = ../cross/bin/x86_64-elf-ld
CC = ../cross/bin/x86_64-elf-gcc -c
INCLUDES = -I../kern/lib/include/shared -I../kern/lib/include/usr
CFLAGS = -O2 $(INCLUDES) -D__DUNKOS_USR
BUILD_DIR = .build
LIBC = $(shell find .. \( -wholename "../kern/lib/shared/*.c" -o -wholename "../kern/lib/usr/*.c" \))
CDIRS = $(shell find . -type d -not -name "$(BUILD_DIR)" -a -not -name ".")
LINKERSRC = ../kern/lib/usr/user.ld

all: reset $(LIBC:%.c=%.o) $(CDIRS:%=%.usr) clean

reset:
	mkdir -p .build
	rm -rf $(BUILD_DIR)/*

%.o: %.c
	$(CC) $(CFLAGS) -o $(BUILD_DIR)/$(notdir $@) $<

%.usr: 
	$(foreach c, $(shell find $* -name "*.c"), $(shell $(CC) $(CFLAGS) -o $(BUILD_DIR)/$*_$(notdir $(c:.c=.o)) $(c)))
	$(LD) -n -T $(LINKERSRC) -o $(BUILD_DIR)/$* $(foreach o, $(patsubst %.c, %.o, $(shell find $* -name "*.c")), $(BUILD_DIR)/$*_$(notdir $(o))) \
	$(foreach libc, $(LIBC:%.c=%.o), $(BUILD_DIR)/$(notdir $(libc)))

clean:
	rm -rf .build/*.o
	