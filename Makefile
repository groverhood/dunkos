

KERN_DIR = ./kern

.PHONY: kern

kern:
	$(MAKE) -C $(KERN_DIR) run