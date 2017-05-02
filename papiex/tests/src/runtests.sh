echo "Watch closely..."
echo ""
echo "Testing papiex with PAPI_TOT_INS and PAPI_TOT_CYC"
echo ""

TESTS="basic fork dlopen exit _exit abort sigint omp pthreads badthreads badpthreads caliper_c caliper_f fullcaliper_c fullcaliper_f testsh testcsh testexec shlogin execl forkexeclp cshlogin execlp system forkexecl"

export LD_LIBRARY_PATH=$PWD/..:$LD_LIBRARY_PATH
export PATH=$PWD:$PATH

for i in $TESTS; do
if [ -x ./$i ]; then
    echo "Running test: $i"
    ../papiex -n -x --classic -e PAPI_TOT_INS -e PAPI_TOT_CYC ./$i < /dev/null
fi
done
