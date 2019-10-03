

KERN_DIR = kern

all: kern

kern:
	$(MAKE) -C $(KERN_DIR) run