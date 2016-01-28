CLEAN_SUBDIRS = src doc tests

BUILD_SUBDIRS = src/threads src/userprog src/vm src/filesys

all::
	@echo "This makefile has only 'clean' and 'check' targets."
	for d in $(BUILD_SUBDIRS); do echo "make -C $$d $@" && $(MAKE) -C $$d $@; done

clean::
	for d in $(CLEAN_SUBDIRS); do $(MAKE) -C $$d $@; done

distclean:: clean
	find . -name '*~' -exec rm '{}' \;

check::
	$(MAKE) -C tests $@
