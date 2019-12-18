

TARGET = x86_64-elf
KERN_DIR = ./kern
TESTDIR = ./tests

.PHONY: kern

kern:
	$(MAKE) -C $(KERN_DIR) run

test:
	julia tests/test.jl