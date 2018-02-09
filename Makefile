CC := gcc
MAILBUGS := papiex-bugs@perftools.org
export CC MAILBUGS

PREFIX := $(PWD)/perftools
DESTPREF := $(PREFIX)

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

.PHONY: install-papiex post-install clean-papiex clobber-papiex clean clobber distclean test fulltest

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
	cp -a env/papiex.sh.in $(DESTPREF)/papiex.sh
	cp -a env/papiex.csh.in $(DESTPREF)/papiex.csh
	cp -a env/papiex.module.in $(DESTPREF)/papiex
	@echo =======================================================================
	@echo "Tools are installed in:"
	@echo $(DESTPREF)
	@echo
	@echo "To use the tools"
	@echo "----------------"
	@echo "module load $(DESTPREF)/papiex"
	@echo "	   - or -"
	@echo "source $(DESTPREF)/papiex.sh"
	@echo "	   - or -"
	@echo "source $(DESTPREF)/papiex.csh"
	@echo
	@echo "To test install:"
	@echo "make test"
	@echo "make fulltest"
	@echo =======================================================================
	@echo

test:
	bash -c 'source $(DESTPREF)/papiex.sh; cd papiex; make quicktest'

fulltest:
	bash -c 'source $(DESTPREF)/papiex.sh; cd papiex; make test'

help:
	@echo "The following targets are supported: "
	@echo
	@echo "    install   - Build and install papiex"
	@echo "    test      - Quick sanity test"
	@echo "    fulltest  - Run all testst"
	@echo "    distclean - Remove all build files and restore to original pristine state"
