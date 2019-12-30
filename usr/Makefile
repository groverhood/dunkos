SOURCE_DIRS = $(wildcard */)

# For each directory, run the mkusr script and append to the specified disk
# image.
all:
	$(foreach sdir, $(SOURCE_DIRS), $(shell julia mkusr.jl -a $(DSKIMG) -d $(sdir)))