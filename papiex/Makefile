DEFINES=-DHAVE_PAPI -DHAVE_MONITOR

LIBNAME=papiex

# If the API changed, bump this and reset all
INTERFACE=1
# If API has been added to but has backwards compat, bump this.
AGE=2
# If anything changed at all bump this.
REVISION=1

VERSION=$(INTERFACE).$(AGE).$(REVISION)

# Put USE_MPI test first as it will change the 
# the compilers to MPICC.
# ifneq (,$(USE_MPI))
# DEFINES += -DUSE_MPI
# include Rules.mpi
# endif

ifeq (1,$(FULL_CALIPER_DATA))
DEFINES += -DFULL_CALIPER_DATA
endif
ifeq (1,$(PROFILING_SUPPORT))
DEFINES += -DPROFILING_SUPPORT
endif
ifeq (1,$(HAVE_BINUTILS))
DEFINES += -DHAVE_BINUTILS
include Rules.binutils
endif

MANPAGES1+=papiex-config.1 papiex.1
MANPAGES3+=papiex_start.3 papiex_stop.3 papiex_accum.3
HTMLMANPAGES1=$(patsubst %.1,%-man.html,$(MANPAGES1))
HTMLMANPAGES3=$(patsubst %.3,%-man.html,$(MANPAGES3))

include Rules.papi
include Rules.monitor
# Rules.gmake sets the sensible defaults
include Rules.gmake


DEFINES += -DPAPIEX_VERSION=\"$(VERSION)\" -I$(root-dir)/src

ifneq (,$(MAILBUGS))
DEFINES += -DMAILBUGS=\"$(MAILBUGS)\"
endif

ifneq (,$(PERFTOOLS_VERSION))
DEFINES += -DPERFTOOLS_VERSION=\"$(PERFTOOLS_VERSION)\"
endif

maketest.output: 
	cd $(build-dir)/tests; ./runtests > ../../$@ 2>&1

.PHONY: install-native
install-native:
	$(MAKE) -C $(build-dir) -f $(root-dir)/src/Makefile install

.PHONY: uninstall-native
uninstall-native:
	$(MAKE) -C $(build-dir) -f $(root-dir)/src/Makefile uninstall

.PHONY: www-native
www-native: 
	-mkdir www/samples
	cp samples/sample.* samples/output.sample www/samples
