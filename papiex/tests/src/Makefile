# Removed omp.c for now. We don't have a compiler for OpenMP.
SRCS = 	badpthreads.c badpthreads2.c close.c \
	basic.c fork.c pthreads.c assert.c abort.c \
	sigint.c exit.c dlopen.c _exit.c write.c puts.c papiex_flops.c \
	fork.c execl.c execlp.c system.c forkexecl.c forkexeclp.c shlogin.c cshlogin.c omp.c
FSRCS = barrier.f90 
SCRIPTS = runtests testexec testcsh testsh verify run_ex_tests bash_empty bash_fork_exec badps
OBJS = $(SRCS:%.c=%.o) $(FSRCS:%.f90=%.o)
NG_TESTS := basic-ng multiplex-ng memory-ng openmp-ng pthreads-ng csv-ng caliper-ng shell-ng syscalls-ng

TESTS = $(OBJS:%.o=%) caliper2 caliper_c caliper_f fullcaliper_c fullcaliper_f gccinstr pthreadgccinstr \
	papiex_thr_flops papiex_thr_instr_flops

TARGETS = $(TESTS) $(SCRIPTS) papiex-tests

ifneq (,$(findstring HAVE_IOEX,$(DEFINES)))
  TARGETS += ioex-tests
  IOEX_TEST_BIN=ioex_writer ioex_thwriter ioex_seek_strided ioex_seek_seq ioex_seek_random
  IOEX_TEST_SCRIPTS=ioex_writer.0.sh ioex_dd.0.sh ioex_thwriter.0.sh ioex_seek_seq.0.sh ioex_seek_strided.0.sh ioex_seek_random.0.sh
  IOEX_TEST_REGEX=$(IOEX_TEST_SCRIPTS:%.sh=%.regex)
endif

CFLAGS += -I$(root-dir)/src
FFLAGS += -I$(root-dir)/src
PAPIEXLIBS += -L$(build-dir) -lpapiex $(LIBS)

PAPIEX_TEST_SCRIPTS=papiex_flops.0.sh papiex_derived_metrics.0.sh papiex_instr.0.sh papiex_thr_instr.0.sh papiex_thr_instr_derived.0.sh papiex_mpi_flops.0.sh

ifneq (,$(findstring USE_MPI,$(DEFINES)))
  MPICC ?= mpicc
  MPIF77 ?= mpif77
  TESTS += papiex_mpi_flops papiex_mpi_thr_flops mpi_pi mpi_wave mpi_exit mpi_hello
  NG_TESTS += mpi-ng
  ifneq (,$(findstring HAVE_MPIP,$(DEFINES)))
    NG_TESTS += mpipex-ng
    MPIEX_TEST_BIN = mpipex_array
    MPIEX_TEST_SCRIPTS = mpipex_array.0.sh
    MPIEX_TEST_REGEX=$(MPIEX_TEST_SCRIPTS:%.sh=%.regex)
    TARGETS += mpipex-tests
  endif
  ifeq (,$(findstring NO_MPI_IO,$(DEFINES)))
    ifneq (,$(findstring HAVE_IOEX,$(DEFINES)))
      IOEX_TEST_BIN += ioex_mpio
      IOEX_TEST_SCRIPTS += ioex_mpio.0.sh
    endif
  endif
endif

ifneq (,$(findstring PROFILING_SUPPORT,$(DEFINES)))
  NG_TESTS += ioprof-ng
  ifneq (,$(findstring USE_MPI,$(DEFINES)))
    NG_TESTS += mpiprof-ng
  endif
endif

ifneq (,$(findstring HAVE_HPCTOOLKIT,$(DEFINES)))
  PAPIEX_TEST_SCRIPTS+=hpcex_thr_flops.0.sh
  NG_TESTS += hpcex-ng
endif
PAPIEX_TEST_REGEX=$(PAPIEX_TEST_SCRIPTS:%.sh=%.regex)

.PHONY: all
all: $(TARGETS)

verify: verify.pl
	cp $< $@

run_ex_tests: run_ex_tests.sh
	cp $< $@

clean-papiex-test:
	-rm -f $(PAPIEX_TEST_SCRIPTS) $(PAPIEX_TEST_REGEX)  2> /dev/null

papiex-tests: $(TESTS) clean-papiex-test _papiex-tests run_ex_tests nextgen-tests

_papiex-tests: $(PAPIEX_TEST_SCRIPTS) $(PAPIEX_TEST_REGEX)
	cp $^ .

clean-ioex-test:
	-rm -f $(IOEX_TEST_SCRIPTS) $(IOEX_TEST_REGEX)  2> /dev/null

_ioex-tests: $(IOEX_TEST_SCRIPTS) $(IOEX_TEST_REGEX)
	cp $^ .

ioex-tests: $(IOEX_TEST_BIN) clean-ioex-test _ioex-tests run_ex_tests

.PHONY: nextgen-tests
nextgen-tests: ng-driver $(NG_TESTS)

%: %.sh
	cp $< $@ && chmod +x $@

clean-mpipex-test:
	-rm -f $(MPIEX_TEST_SCRIPTS) $(MPIEX_TEST_REGEX)  2> /dev/null

_mpipex-tests: $(MPIEX_TEST_SCRIPTS) $(MPIEX_TEST_REGEX)
	cp $^ .

mpipex-tests: $(MPIEX_TEST_BIN) clean-mpipex-test _mpipex-tests run_ex_tests

runtests: runtests.sh
	cp $< $@

testexec: testexec.pl
	cp $< $@

testsh: testsh.sh
	cp $< $@

testcsh: testcsh.csh
	cp $< $@

papiex_thr_flops: papiex_flops.c
	$(CC_PTHR) -DHAVE_PTHREADS -g $< -o $@

papiex_thr_instr_flops: papiex_flops.c
	$(CC_PTHR) -finstrument-functions -DHAVE_PTHREADS -g $< -o $@

pi: pi.f
	-$(MPIF77) -g $< -o $@

mpi_%: mpi_%.c
	-$(MPICC) -g -O2 $< -o $@

mpi_wave: mpi_wave.f
	-$(MPIF77) -g -O2 $< -o $@

mpi_pi: mpi_pi.c
	-$(MPICC) -g -O2 $< -o $@

mpi_exit: mpi_exit.c
	-$(MPICC) -g -O2 $< -o $@

mpi_hello: mpi_hello.c
	-$(MPICC) -g -O2 $< -o $@

mpipex_array: mpipex_array.c
	-$(MPICC) -g $< -o $@

papiex_mpi_flops: papiex_flops.c
	-$(MPICC) -DUSE_MPI -g $< -o $@

papiex_mpi_thr_flops: papiex_flops.c
	-$(MPICC) -DUSE_MPI -DHAVE_PTHREADS -g $< -lpthread -o $@

ioex_mpio: ioex_mpio.c
	-$(MPICC) $(CFLAGS) $< -o $@

ioex_thwriter: ioex_thwriter.c
	$(CC_PTHR) $(CFLAGS) $< -o $@

dlopen: dlopen.c
	$(CC) $(CFLAGS) $< -o $@ -ldl

gccinstr: instr.c
	-$(CC) -g -finstrument-functions $< -o $@

pthreadgccinstr: pthreads.c
	-$(CC_PTHR) -g -finstrument-functions $< -o $@

pthreads badpthreads badpthreads2: %: %.c
	-$(CC_PTHR) $(CFLAGS) $< -o $@

omp: omp.c
	-$(CC_OMP) $(CFLAGS) $< -o $@

barrier: barrier.f90
	-$(F77_OMP) $(FFLAGS) $< -o $@

caliper2: caliper2.c
	$(CC) $(CFLAGS) $< -o $@ $(PAPIEXLIBS) -lm

caliper_c: caliper.c
	$(CC) -DFULL_CALIPER $(CFLAGS) $< -o $@ $(PAPIEXLIBS) -lm

caliper_f: caliper.F
	-$(F77) $(FFLAGS) $< -o $@ $(PAPIEXLIBS)

fullcaliper_c: caliper.c
	$(CC) $(CFLAGS) -DFULL_CALIPER $< -o $@ $(PAPIEXLIBS) -lm

fullcaliper_f: caliper.F
	-$(F77) $(FFLAGS) -DFULL_CALIPER $< -o $@ $(PAPIEXLIBS)

.PHONY: clean
clean: clean-mpipex-test clean-ioex-test clean-papiex-test
	-rm -f *~ core.* *.o *.a *.sh *.regex *.out 2>/dev/null

.PHONY: clobber
clobber:
	-rm -f $(TARGETS)
