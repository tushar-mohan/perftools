PERFTOOLS_VERSION=1.2.1
DISTNAME=perftools-$(PERFTOOLS_VERSION)

MAILBUGS := perftools-bugs@perftools.org

ifeq (,$(PREFIX))
  PREFIX = $(PWD)/$(DISTNAME)
endif
DESTPREF = $(PREFIX)

CC := gcc
export CC MAILBUGS PERFTOOLS_VERSION


LIBMONITOR := $(DESTPREF)/lib/libmonitor.so
LIBPAPI := $(DESTPREF)/lib/libpapi.so

# are we building monitor/papi? Or using a user-supplied one?
# If the user has set MONITOR_PREFIX or MONITOR_INC_PATH+MONITOR_LIB_PATH
# then we do not build monitor. Same goes for PAPI
DEPS =
ifeq (,$(MONITOR_PREFIX))
  ifeq (,$(MONITOR_INC_PATH))
    ifeq (,$(MONITOR_LIB_PATH))
      DEPS += $(LIBMONITOR)
    endif
  endif
endif
ifeq (,$(PAPI_PREFIX))
  ifeq (,$(PAPI_INC_PATH))
    ifeq (,$(PAPI_LIB_PATH))
      DEPS += $(LIBPAPI)
    endif
  endif
endif

PAPI_PREFIX := $(DESTPREF)
PAPI_INC_PATH ?= $(PAPI_PREFIX)/include
PAPI_LIB_PATH ?= $(PAPI_PREFIX)/lib

MONITOR_PREFIX := $(DESTPREF)
MONITOR_INC_PATH ?= $(MONITOR_PREFIX)/include
MONITOR_LIB_PATH ?= $(MONITOR_PREFIX)/lib


ifneq (,$(findstring $(LIBMONITOR),$(DEPS)))
    include incl/Makefile.monitor
endif
ifneq (,$(findstring $(LIBPAPI),$(DEPS)))
    include incl/Makefile.papi
endif

include incl/Makefile.papiex
include incl/Makefile.libunwind
include incl/Makefile.hpctoolkit

.PHONY: install all install-papiex post-install clean-papiex clobber-papiex clean clobber distclean mrproper test fulltest

install: install-papiex post-install

all: install-papiex install-libunwind install-hpctoolkit-externals install-hpctoolkit post-install


clean: clean-papiex clean-hpctoolkit clean-hpctoolkit-externals clean-libunwind
	@if [ -d papi ];then $(MAKE) clean-papi; fi
	@if [ -d monitor ];then $(MAKE) clean-monitor; fi

clobber: clean
	@if [ -d papi ]; then $(MAKE) clobber-papi; fi
	@if [ -d monitor ]; then $(MAKE) clobber-monitor; fi
	@if [ -d hpctoolkit ]; then $(MAKE) clobber-hpctoolkit; fi
	@if [ -d hpctoolkit-externals ]; then $(MAKE) clobber-hpctoolkit-externals; fi
	@if [ -d libunwind ]; then $(MAKE) clobber-libunwind; fi

distclean mrproper: clobber
	@rm -rf papi hpctoolkit hpctoolkit-externals libunwind $(DESTPREF)

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

test:
	bash -c 'source $(DESTPREF)/perftools.sh; cd papiex; make quicktest'

fulltest:
	bash -c 'source $(DESTPREF)/pperftools.sh; cd papiex; make test'

help:
	@echo "The following targets are supported: "
	@echo
	@echo "    install   - Build and install papiex"
	@echo "    test      - Quick sanity test"
	@echo "    fulltest  - Run all testst"
	@echo "    distclean - Remove all build files and restore to original pristine state"
