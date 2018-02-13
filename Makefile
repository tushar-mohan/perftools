PERFTOOLS_VERSION=1.2.1
DISTNAME=perftools-$(PERFTOOLS_VERSION)

MAILBUGS := perftools-bugs@perftools.org

ifeq (,$(PREFIX))
  PREFIX = $(PWD)/$(DISTNAME)
endif
DESTPREF = $(PREFIX)

CC := gcc
export CC MAILBUGS PERFTOOLS_VERSION

.PHONY:
install: install-papiex post-install

include incl/Makefile.papiex
include incl/Makefile.libunwind
include incl/Makefile.hpctoolkit

.PHONY:
all: install-papiex install-libunwind install-hpctoolkit-externals install-hpctoolkit post-install

.PHONY:
post-install:
	cp -a env/perftools.sh.in $(DESTPREF)/perftools.sh
	cp -a env/perftools.csh.in $(DESTPREF)/perftools.csh
	cp -a env/perftools.module.in $(DESTPREF)/perftools
	@echo =======================================================================
	@echo "Tools are installed in:"
	@echo $(DESTPREF)
	@echo
	@echo "To use the tools"
	@echo "----------------"
	@echo "module load $(DESTPREF)/perftools"
	@echo "	   - or -"
	@echo "source $(DESTPREF)/perftools.sh"
	@echo "	   - or -"
	@echo "source $(DESTPREF)/perftools.csh"
	@echo
	@echo "To test install:"
	@echo "make test"
	@echo "make fulltest"
	@echo =======================================================================
	@echo

.PHONY:
test:
	bash -c 'source $(DESTPREF)/perftools.sh; cd papiex; make quicktest'

.PHONY:
fulltest:
	bash -c 'source $(DESTPREF)/pperftools.sh; cd papiex; make test'

.PHONY:
help:
	@echo "The following targets are supported: "
	@echo
	@echo "    install   - Build and install papiex"
	@echo "    all       - Build and install all tools, including hpctoolkit (15-20 minutes)"
	@echo "    test      - Quick sanity test"
	@echo "    fulltest  - Run all testst"
	@echo "    distclean - Remove all build files and restore to original pristine state"

.PHONY:
clean: clean-papiex

.PHONY:
clobber: clobber-papiex

.PHONY: 
clean-all: 
	@for tool in papiex papi monitor hpctoolkit hpctoolkit-externals libunwind; do [ ! -d $$tool ] || $(MAKE) clean-$$tool; done

.PHONY:
clobber-all: clean-all
	@for tool in papiex papi monitor hpctoolkit hpctoolkit-externals libunwind; do [ ! -d $$tool ] || $(MAKE) clobber-$$tool; done

.PHONY:
distclean mrproper: clobber-all
	@rm -rf papi hpctoolkit hpctoolkit-externals libunwind $(DESTPREF)
