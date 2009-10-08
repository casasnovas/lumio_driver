DESTDIR := /lib/modules/$(shell uname -r)

PROJECT_NAME=lumio_driver
PROJECT_VERSION=0.1

all: lumio_driver

lumio_driver:
	make -C src/
	cp src/lumio_driver.ko ./

.PHONY: doc

doc:
	doxygen Doxyfile

install: all
	mkdir -p $(DESTDIR)/misc
	install lumio_driver.ko $(DESTDIR)/misc/
	depmod -a
	modprobe lumio_driver
	@echo "*****"
	@echo "The driver won't be loaded at next boot, follow the instructions (see INSTALL) for your distribution to make it automatically loading at next boot."
	@echo "*****"

uninstall:
	rmmod lumio_driver
	rm $(DESTDIR)/misc/lumio_driver.ko

tarball: clean
	git archive --format=tar --prefix=$(PROJECT_NAME)-$(PROJECT_VERSION) HEAD > $(PROJECT_NAME)-$(PROJECT_VERSION).tar
	bzip2 $(PROJECT_NAME)-$(PROJECT_VERSION).tar

clean:
	make -C src/ clean
	rm -f lumio_driver.ko
	rm -Rf doc/*