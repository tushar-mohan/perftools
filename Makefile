CC := gcc
export CC

PREFIX := $(PWD)/perftools-$(shell date +\%Y\%m\%d)
DESTPREF := $(PREFIX)

LIBMONITOR := $(DESTPREF)/lib/libmonitor.so
LIBPAPI := $(DESTPREF)/lib/libpapi.so

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

# disabled PROFILING_SUPPORT
install-papiex: $(DEPS)
	cd papiex; $(MAKE) CC=$(CC) OCC=$(OCC) FULL_CALIPER_DATA=1 MONITOR_INC_PATH=$(MONITOR_INC_PATH) MONITOR_LIB_PATH=$(MONITOR_LIB_PATH) PAPI_INC_PATH=$(PAPI_INC_PATH) PAPI_LIB_PATH=$(PAPI_LIB_PATH) PREFIX=$(DESTPREF) install


.PHONY: clean
clean: 
	cd papiex; $(MAKE) clean
	@rm -rf papiex/x86_64-Linux
	@if [ -d papi ];then $(MAKE) clean-papi; fi
	@if [ -d monitor ];then $(MAKE) clean-monitor; fi

.PHONY: clobber
clobber: clean
	@rm -rf papiex-install
	@if [ -d papi ]; then $(MAKE) clobber-papi; fi
	@if [ -d monitor ]; then $(MAKE) clobber-monitor; fi

.PHONY: distclean mrproper
distclean mrproper: clobber
	@rm -rf papi perftools-*

.PHONY: post-install
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

.PHONY: test
test:
	bash -c 'source $(DESTPREF)/papiex.sh; cd papiex; make quicktest'

.PHONY: fulltest
fulltest:
	bash -c 'source $(DESTPREF)/papiex.sh; cd papiex; make test'
