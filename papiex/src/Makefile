# $Id: Makefile 74977 2009-04-13 09:46:10Z tmohan $

TARGETS += papiex papiex-config $(SONAME) papiex-specs papiex-report papiex-output2csv

ifneq (,$(findstring PROFILING_SUPPORT,$(DEFINES)))
  TARGETS += libpapiexio.so libpapiexmpsync.so
  ifneq (,$(findstring USE_MPI,$(DEFINES)))
    TARGETS += libpapiexmpi.so 
  endif
endif

OBJS=papiex.o eventinfo.o memory.o 
ifneq (,$(findstring HAVE_BINUTILS,$(DEFINES)))
  TARGETS += libpapiexpathscale.so libpapiexgcc.so 
  OBJS += pclookup.o
endif
LDFLAGS+=-Wl,-rpath -Wl,$(LIBDIR) -L$(build-dir) $(LIBS) -lm

.PHONY: all
all: $(TARGETS)

threadsyncwrap.c: threadsyncwrap.in makewraplib.pl
	perl $(root-dir)/src/makewraplib.pl < $(root-dir)/src/threadsyncwrap.in > $@

iowrap.c: iowrap.in makewraplib.pl
	perl $(root-dir)/src/makewraplib.pl < $(root-dir)/src/iowrap.in > $@

mpiwrap.c: mpiwrap_special.c mpiwrap.in makempiwraplib.pl
	perl $(root-dir)/src/makempiwraplib.pl -f < $(root-dir)/src/mpiwrap.in > $@
	cat $(root-dir)/src/mpiwrap_special.c >> $@

libiberty.so: $(LIBIBERTY_STATIC)
	$(LD) -rpath $(LIBDIR) -whole-archive -soname $@ $< -G -o $@

libbfd.so: $(LIBBFD_STATIC)
	$(LD) -rpath $(LIBDIR) -whole-archive -soname $@ $< -G -o $@

libpapiexpathscale.so: pathscale.c 
	$(CC) $(CFLAGS) $(DEFINES) -DUSE_PAPIEX $(SHRCFLAGS) $(SHRLDFLAGS) $^ -Wl,-soname -Wl,$@.$(INTERFACE) $(LDFLAGS) -ldl -o $@.$(INTERFACE).$(REVISION).$(AGE) && \
	ln -sf $@.$(INTERFACE).$(REVISION).$(AGE) $@.$(INTERFACE) && \
	ln -sf $@.$(INTERFACE) $@

libpapiexgcc.so: gcc.c
	$(CC) $(CFLAGS) $(DEFINES) -DUSE_PAPIEX $(SHRCFLAGS) $(SHRLDFLAGS) $^ -Wl,-soname -Wl,$@.$(INTERFACE) $(LDFLAGS) -ldl -o $@.$(INTERFACE).$(REVISION).$(AGE) && \
	ln -sf $@.$(INTERFACE).$(REVISION).$(AGE) $@.$(INTERFACE) && \
	ln -sf $@.$(INTERFACE) $@

libpapiexio.so: iowrap.c
	$(CC) $(CFLAGS) $(DEFINES) $(SHRCFLAGS) $(SHRLDFLAGS) $^ -Wl,-soname -Wl,$@.$(INTERFACE) $(LDFLAGS) -ldl -o $@.$(INTERFACE).$(REVISION).$(AGE) && \
	ln -sf $@.$(INTERFACE).$(REVISION).$(AGE) libpapiexio.so.$(INTERFACE) && \
	ln -sf libpapiexio.so.$(INTERFACE) libpapiexio.so

libpapiexmpsync.so: threadsyncwrap.c
	$(CC) $(CFLAGS) $(DEFINES) $(SHRCFLAGS) $(SHRLDFLAGS) $^ -Wl,-soname -Wl,$@.$(INTERFACE) $(LDFLAGS) -ldl -o $@.$(INTERFACE).$(REVISION).$(AGE) && \
	ln -sf $@.$(INTERFACE).$(REVISION).$(AGE) libpapiexmpsync.so.$(INTERFACE) && \
	ln -sf libpapiexmpsync.so.$(INTERFACE) libpapiexmpsync.so

libpapiexmpi.so: mpiwrap.c
	$(MPICC) $(CFLAGS) $(DEFINES) $(SHRCFLAGS) $(SHRLDFLAGS) $^ -Wl,-soname -Wl,$@.$(INTERFACE) $(LDFLAGS) -ldl -o $@.$(INTERFACE).$(REVISION).$(AGE) && \
	ln -sf $@.$(INTERFACE).$(REVISION).$(AGE) libpapiexmpi.so.$(INTERFACE) && \
	ln -sf libpapiexmpi.so.$(INTERFACE) libpapiexmpi.so

papiex-report: papiex-report.pl
	cp $< $@

papiex-output2csv: papiex-output2csv.py
	cp $< $@

papiex: main.c eventinfo.o
	$(CC) $(CFLAGS) $(DEFINES) $^ -o $@ $(LDFLAGS)

# make sure you link libpapiex with 
# static libbfd and libiberty to avoid unnecessary
# dependencies on external shared libraries
$(SONAME): $(OBJS) $(LIBBFD_STATIC) $(LIBIBERTY_STATIC)
	$(CC_PTHR) $(CFLAGS) $(DEFINES) $(SHRLDFLAGS) $(OBJS) -Wl,-soname -Wl,$(SOVERSION) $(LDFLAGS) $(LIBBFD_STATIC) $(LIBIBERTY_STATIC) -ldl -lmonitor -lpthread -o $@ && \
	ln -sf $(SONAME) $(SOVERSION) && \
	ln -sf $(SOVERSION) $(SOBASE) 

$(OBJS): %.o: %.c papiex_internal.h pathscale.h papiex.h
	$(CC_PTHR) $(CFLAGS) $(DEFINES) $(SHRCFLAGS) -c $<

papiex-config: papiex-config.in
	@sed -e 's\PREFIX\$(PREFIX)\g; s\EXECPREFIX\$(EXECPREFIX)\g; s\INCDIR\$(INCDIR)\g; s\LIBDIR\$(LIBDIR)\g; s\MANDIR\$(MANDIR)\g' < $< > $@
	@chmod +x papiex-config

papiex-specs: specs
	-rm -frv $@
	cp -r $< $@

install: all
	install $(build-dir)/libpapiex.so.$(INTERFACE).$(REVISION).$(AGE) $(DESTDIR)$(LIBDIR)
	cd $(DESTDIR)$(LIBDIR); ln -fs libpapiex.so.$(INTERFACE).$(REVISION).$(AGE) libpapiex.so.$(INTERFACE)
	cd $(DESTDIR)$(LIBDIR); ln -fs libpapiex.so.$(INTERFACE) libpapiex.so
ifneq (,$(findstring libpapiexmpi.so,$(TARGETS)))
	install $(build-dir)/libpapiexmpi.so.$(INTERFACE).$(REVISION).$(AGE) $(DESTDIR)$(LIBDIR)
	cd $(DESTDIR)$(LIBDIR); ln -fs libpapiexmpi.so.$(INTERFACE).$(REVISION).$(AGE) libpapiexmpi.so.$(INTERFACE)
	cd $(DESTDIR)$(LIBDIR); ln -fs libpapiexmpi.so.$(INTERFACE) libpapiexmpi.so
endif
ifneq (,$(findstring libpapiexio.so,$(TARGETS)))
	install $(build-dir)/libpapiexio.so.$(INTERFACE).$(REVISION).$(AGE) $(DESTDIR)$(LIBDIR)
	cd $(DESTDIR)$(LIBDIR); ln -fs libpapiexio.so.$(INTERFACE).$(REVISION).$(AGE) libpapiexio.so.$(INTERFACE)
	cd $(DESTDIR)$(LIBDIR); ln -fs libpapiexio.so.$(INTERFACE) libpapiexio.so
endif
ifneq (,$(findstring libpapiexmpsync.so,$(TARGETS)))
	install $(build-dir)/libpapiexmpsync.so.$(INTERFACE).$(REVISION).$(AGE) $(DESTDIR)$(LIBDIR)
	cd $(DESTDIR)$(LIBDIR); ln -fs libpapiexmpsync.so.$(INTERFACE).$(REVISION).$(AGE) libpapiexmpsync.so.$(INTERFACE)
	cd $(DESTDIR)$(LIBDIR); ln -fs libpapiexmpsync.so.$(INTERFACE) libpapiexmpsync.so
endif
ifneq (,$(findstring libpapiexpathscale.so,$(TARGETS)))
	install $(build-dir)/libpapiexpathscale.so.$(INTERFACE).$(REVISION).$(AGE) $(DESTDIR)$(LIBDIR)
	cd $(DESTDIR)$(LIBDIR); ln -fs libpapiexpathscale.so.$(INTERFACE).$(REVISION).$(AGE) libpapiexpathscale.so.$(INTERFACE)
	cd $(DESTDIR)$(LIBDIR); ln -fs libpapiexpathscale.so.$(INTERFACE) libpapiexpathscale.so
endif
ifneq (,$(findstring libpapiexgcc.so,$(TARGETS)))
	install $(build-dir)/libpapiexgcc.so.$(INTERFACE).$(REVISION).$(AGE) $(DESTDIR)$(LIBDIR)
	cd $(DESTDIR)$(LIBDIR); ln -fs libpapiexgcc.so.$(INTERFACE).$(REVISION).$(AGE) libpapiexgcc.so.$(INTERFACE)
	cd $(DESTDIR)$(LIBDIR); ln -fs libpapiexgcc.so.$(INTERFACE) libpapiexgcc.so
endif
	install $(build-dir)/papiex $(DESTDIR)$(BINDIR)
	install $(build-dir)/papiex-report $(DESTDIR)$(BINDIR)
	install $(build-dir)/papiex-output2csv $(DESTDIR)$(BINDIR)
	install -d $(DESTDIR)$(PREFIX)/share/papiex
	cp -v $(build-dir)/papiex-specs/* $(DESTDIR)$(PREFIX)/share/papiex/

uninstall:
	-cd $(DESTDIR)$(LIBDIR); rm -f libpapiexio.so libpapiexio.so.$(INTERFACE) libpapiexio.so.$(INTERFACE).$(REVISION).$(AGE)
	-cd $(DESTDIR)$(LIBDIR); rm -f libpapiexmpsync.so libpapiexmpsync.so.$(INTERFACE) libpapiexmpsync.so.$(INTERFACE).$(REVISION).$(AGE)
ifneq (,$(findstring libpapiexmpi.so,$(TARGETS)))
	-cd $(DESTDIR)$(LIBDIR); rm -f libpapiexmpi.so libpapiexmpi.so.$(INTERFACE) libpapiexmpi.so.$(INTERFACE).$(REVISION).$(AGE)
endif
	-cd $(DESTDIR)$(BINDIR); rm -f papiex
