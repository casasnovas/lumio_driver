SHELL := /bin/sh
SRCDIR := $(shell pwd)
DESTDIR := /usb/bin
MODDIR := /lib/modules/$(shell uname -r)

PROJECT_NAME=lumio_driver
PROJECT_VERSION=0.3

all: lumio_driver helper check

print_src_dir:
	@echo $(SRCDIR)

lumio_driver:
	make -C src/
	mv $(SRCDIR)/src/lumio_driver.ko ./

helper:
	make -C $(SRCDIR)/helper/
	mv $(SRCDIR)/helper/detach ./
	cp $(SRCDIR)/helper/lumio_load_driver ./

check:
	make -C check/
	cp $(SRCDIR)/check/lumio_create_cursors ./
	mv $(SRCDIR)/check/draw_mice ./

.PHONY: doc helper check

doc:
	doxygen Doxyfile

install: all
	install ./draw_mice $(DEST_DIR)
	install ./lumio_create_cursors $(DEST_DIR)
	install ./detach $(DEST_DIR)
	install ./lumio_load_driver $(DEST_DIR)
	mkdir -p $(MODDIR)/misc
	install ./lumio_driver.ko $(MODDIR)/misc/
	depmod -a
	@echo "Lumio driver is installed, you still have to load it (using lumio_load_driver commad)."

uninstall:
	rm $(MODDIR)/misc/lumio_driver.ko
	rm $(DESTDIR)/{detach, lumio_load_driver, lumio_create_cursors, draw_mice}

tarball:
	git archive --format=tar --prefix=$(PROJECT_NAME)-$(PROJECT_VERSION)/ master > $(PROJECT_NAME)-$(PROJECT_VERSION).tar
	bzip2 $(PROJECT_NAME)-$(PROJECT_VERSION).tar

clean:
	make -C check/ clean
	make -C helper/ clean
	make -C src/ clean
	rm -f ./lumio_driver.ko
	rm -f ./draw_mice
	rm -f ./detach
	rm -f ./lumio_create_cursors
	rm -f ./lumio_load_driver
	rm -Rf doc/*