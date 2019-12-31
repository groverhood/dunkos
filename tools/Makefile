BUILD_DIR = .build
CC = /usr/bin/cc
LD = /usr/bin/ld
CFLAGS = -O2 -fPIC -c -Wall
LFLAGS = -shared

LIBTOOLS_SRCS = $(shell find . -name *.c)
LIBTOOLS_OBJS = $(patsubst ./%.c, $(BUILD_DIR)/%.o, $(LIBTOOLS_SRCS))

all: clean builddir $(LIBTOOLS_OBJS)
	$(LD) $(LFLAGS) -o $(BUILD_DIR)/dtools.so $(LIBTOOLS_OBJS)

builddir:
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)

$(BUILD_DIR)/%.o: %.c
	$(CC) $(CFLAGS) -o $@ $<
