
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

install: install-papiex post-install

ifneq (,$(findstring $(LIBMONITOR),$(DEPS)))
    include incl/Makefile.monitor
endif
ifneq (,$(findstring $(LIBPAPI),$(DEPS)))
    include incl/Makefile.papi
endif

.PHONY: minimal all install-papiex post-install clean-papiex clobber-papiex clean clobber distclean mrproper test fulltest

minimal: install-papiex

all: minimal install-hpctoolkit

install-papiex: $(DEPS)
	cd papiex; $(MAKE) CC=$(CC) OCC=$(OCC) FULL_CALIPER_DATA=1 PROFILING_SUPPORT=1 MONITOR_INC_PATH=$(MONITOR_INC_PATH) MONITOR_LIB_PATH=$(MONITOR_LIB_PATH) PAPI_INC_PATH=$(PAPI_INC_PATH) PAPI_LIB_PATH=$(PAPI_LIB_PATH) PREFIX=$(DESTPREF) MAILBUGS=$(MAILBUGS) install

clean-papiex:
	cd papiex; $(MAKE) clean

clean: clean-papiex
	@if [ -d papi ];then $(MAKE) clean-papi; fi
	@if [ -d monitor ];then $(MAKE) clean-monitor; fi

clobber-papiex:
	@rm -rf papiex/x86_64-Linux

clobber: clean clobber-papiex
	@if [ -d papi ]; then $(MAKE) clobber-papi; fi
	@if [ -d monitor ]; then $(MAKE) clobber-monitor; fi

distclean mrproper: clobber
	@rm -rf papi $(DESTPREF)

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
