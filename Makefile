

TARGET = x86_64-elf
KERN_DIR = ./kern
PYTHON = python3
TESTDIR = ./tests

.PHONY: kern

kern:
	$(MAKE) -C $(KERN_DIR) run

test:
	$(PYTHON) $(TESTDIR)/test.py -cc ./cross/bin/$(TARGET)-gcc