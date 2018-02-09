# $Id: Makefile 74977 2009-04-13 09:46:10Z tmohan $

TARGETS += papiex papiex-config $(SONAME) papiex-specs papiex-report papiex-output2csv

VERSION=$(INTERFACE).$(REVISION).$(AGE)

ifneq (,$(findstring PROFILING_SUPPORT,$(DEFINES)))
  TARGETS += libpapiexio.so.$(VERSION) libpapiexmpsync.so.$(VERSION) libpapiexmpi.so.$(VERSION)
endif

OBJS=papiex.o eventinfo.o memory.o 
ifneq (,$(findstring HAVE_BINUTILS,$(DEFINES)))
  TARGETS += libpapiexpathscale.so.$(VERSION) libpapiexgcc.so.$(VERSION) 
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

libpapiexpathscale.so.$(VERSION): TMP_SOBASE = libpapiexpathscale.so
libpapiexpathscale.so.$(VERSION): pathscale.c
	$(CC) $(CFLAGS) $(DEFINES) -DUSE_PAPIEX $(SHRCFLAGS) $(SHRLDFLAGS) $^ -Wl,-soname -Wl,$(TMP_SOBASE).$(INTERFACE) $(LDFLAGS) -ldl -o $@ && \
	ln -sf $@ $(TMP_SOBASE).$(INTERFACE) && \
	ln -sf $(TMP_SOBASE).$(INTERFACE) $(TMP_SOBASE)

libpapiexgcc.so.$(VERSION): TMP_SOBASE = libpapiexgcc.so
libpapiexgcc.so.$(VERSION): gcc.c
	$(CC) $(CFLAGS) $(DEFINES) -DUSE_PAPIEX $(SHRCFLAGS) $(SHRLDFLAGS) $^ -Wl,-soname -Wl,$(TMP_SOBASE).$(INTERFACE) $(LDFLAGS) -ldl -o $@ && \
	ln -sf $@ $(TMP_SOBASE).$(INTERFACE) && \
	ln -sf $(TMP_SOBASE).$(INTERFACE) $(TMP_SOBASE)

libpapiexio.so.$(VERSION): TMP_SOBASE = libpapiexio.so
libpapiexio.so.$(VERSION): iowrap.c
	$(CC) $(CFLAGS) $(DEFINES) $(SHRCFLAGS) $(SHRLDFLAGS) $^ -Wl,-soname -Wl,$(TMP_SOBASE).$(INTERFACE) $(LDFLAGS) -ldl -o $@ && \
	ln -sf $@ $(TMP_SOBASE).$(INTERFACE) && \
	ln -sf $(TMP_SOBASE).$(INTERFACE) $(TMP_SOBASE)

libpapiexmpsync.so.$(VERSION): TMP_SOBASE = libpapiexmpsync.so
libpapiexmpsync.so.$(VERSION): threadsyncwrap.c
	$(CC) $(CFLAGS) $(DEFINES) $(SHRCFLAGS) $(SHRLDFLAGS) $^ -Wl,-soname -Wl,$(TMP_SOBASE).$(INTERFACE) $(LDFLAGS) -ldl -o $@ && \
	ln -sf $@ $(TMP_SOBASE).$(INTERFACE) && \
	ln -sf $(TMP_SOBASE).$(INTERFACE) $(TMP_SOBASE)

libpapiexmpi.so.$(VERSION): TMP_SOBASE = libpapiexmpi.so
libpapiexmpi.so.$(VERSION): mpiwrap.c
	$(MPICC) $(CFLAGS) $(DEFINES) $(SHRCFLAGS) $(SHRLDFLAGS) $^ -Wl,-soname -Wl,$(TMP_SOBASE).$(INTERFACE) $(LDFLAGS) -ldl -o $@ && \
	ln -sf $@ $(TMP_SOBASE).$(INTERFACE) && \
	ln -sf $(TMP_SOBASE).$(INTERFACE) $(TMP_SOBASE)

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
	install $(build-dir)/libpapiex.so.$(VERSION) $(DESTDIR)$(LIBDIR)
	cd $(DESTDIR)$(LIBDIR); ln -fs libpapiex.so.$(VERSION) libpapiex.so.$(INTERFACE)
	cd $(DESTDIR)$(LIBDIR); ln -fs libpapiex.so.$(INTERFACE) libpapiex.so
ifneq (,$(findstring libpapiexmpi.so,$(TARGETS)))
	install $(build-dir)/libpapiexmpi.so.$(INTERFACE).$(REVISION).$(AGE) $(DESTDIR)$(LIBDIR)
	cd $(DESTDIR)$(LIBDIR); ln -fs libpapiexmpi.so.$(INTERFACE).$(REVISION).$(AGE) libpapiexmpi.so.$(INTERFACE)
	cd $(DESTDIR)$(LIBDIR); ln -fs libpapiexmpi.so.$(INTERFACE) libpapiexmpi.so
endif
ifneq (,$(findstring libpapiexio.so,$(TARGETS)))
	install $(build-dir)/libpapiexio.so.$(VERSION) $(DESTDIR)$(LIBDIR)
	cd $(DESTDIR)$(LIBDIR); ln -fs libpapiexio.so.$(VERSION) libpapiexio.so.$(INTERFACE)
	cd $(DESTDIR)$(LIBDIR); ln -fs libpapiexio.so.$(INTERFACE) libpapiexio.so
endif
ifneq (,$(findstring libpapiexmpsync.so,$(TARGETS)))
	install $(build-dir)/libpapiexmpsync.so.$(VERSION) $(DESTDIR)$(LIBDIR)
	cd $(DESTDIR)$(LIBDIR); ln -fs libpapiexmpsync.so.$(VERSION) libpapiexmpsync.so.$(INTERFACE)
	cd $(DESTDIR)$(LIBDIR); ln -fs libpapiexmpsync.so.$(INTERFACE) libpapiexmpsync.so
endif
ifneq (,$(findstring libpapiexpathscale.so,$(TARGETS)))
	install $(build-dir)/libpapiexpathscale.so.$(VERSION) $(DESTDIR)$(LIBDIR)
	cd $(DESTDIR)$(LIBDIR); ln -fs libpapiexpathscale.so.$(VERSION) libpapiexpathscale.so.$(INTERFACE)
	cd $(DESTDIR)$(LIBDIR); ln -fs libpapiexpathscale.so.$(INTERFACE) libpapiexpathscale.so
endif
ifneq (,$(findstring libpapiexgcc.so,$(TARGETS)))
	install $(build-dir)/libpapiexgcc.so.$(VERSION) $(DESTDIR)$(LIBDIR)
	cd $(DESTDIR)$(LIBDIR); ln -fs libpapiexgcc.so.$(VERSION) libpapiexgcc.so.$(INTERFACE)
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
	-cd $(DESTDIR)$(LIBDIR); rm -f libpapiexmpi.so libpapiexmpi.so.$(INTERFACE) libpapiexmpi.so.$(VERSION)
endif
	-cd $(DESTDIR)$(BINDIR); rm -f papiex
