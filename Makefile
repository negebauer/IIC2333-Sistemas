CLEAN_SUBDIRS = src doc tests
BUILD_SUBDIRS = src/threads src/userprog src/vm src/filesys
HAS_CTAGS := $(shell which ctags 2> /dev/null)


all::
ifdef HAS_CTAGS
	@echo "Building tags..."
	ctags -R
	ctags -e -R
	@echo ""
else
	echo "Install ctags to easily navigate the source"
endif
	for d in $(BUILD_SUBDIRS); do echo "Building $$d..." && $(MAKE) -C $$d $@ && echo ""; done

clean::
	for d in $(CLEAN_SUBDIRS); do $(MAKE) -C $$d $@; done

distclean:: clean
	find . -name '*~' -exec rm '{}' \;

check::
	$(MAKE) -C tests $@
