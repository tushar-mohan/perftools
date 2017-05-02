#!/usr/bin/perl

sub getargs
{
    my $input = shift;
    my @al = split ',',$input;

    my $i = 0;
    my $args = "";
    while ($i <= $#al)
    {
	my @bl = split ' ',$al[$i];
	my $tmp = $bl[$#bl];
	if ($tmp =~ /([a-z_A-Z]+)/) {
	    $args .= "$1"; 
	    if ($i <= $#al - 1) {
	    $args .= ","; 
	    }
	    }
	$i++;
    }
    return $args;
}

sub trim($)
{
    my $string = shift;
    $string =~ s/^\s+//;
    $string =~ s/\s+$//;
    return $string;
}

sub make_fortran_proto
{
    my $input = shift;
    my @args = split ',',$input;
    my $j = 0;
    while ($j <= $#args) {
	$args[$j] = trim($args[$j]);
#    printf "processing arg |%s|\n",$args[$j];
	my @terms = split ' ',$args[$j];
	my $k = $#terms;
	if (!($terms[$k] =~ /^\*.*/)) {
#	    $ftnptrarg[$j] = 1; 
	    $terms[$k] = "*" . $terms[$k];
	    my $ftnarg = join(' ', @terms);
#	    printf "arg %d needs pointer, term |%s|, new arg |%s|\n",$j,$terms[$k],$ftnarg;
	    $args[$j] = $ftnarg;
	}
	$j++;
    }
    return join(', ',@args);
}

sub make_fortran_call2c
{
    my $input = shift;
    my @args = split ',',$input;
    my $j = 0;
    my @ftcargs = split ',',getargs($input);
#    print @ftcargs, "\n";
    while ($j <= $#args) {
	$args[$j] = trim($args[$j]);
#    printf "processing arg |%s|\n",$args[$j];
	my @terms = split ' ',$args[$j];
	my $k = $#terms;
	if (!($terms[$k] =~ /^\*.*/)) {
#	    $ftnptrarg[$j] = 1; 
	    $terms[$k] = "*" . $terms[$k];
	    my $ftnarg = join(' ', @terms);
#	    printf "arg %d needs pointer, term |%s|, new arg |%s|\n",$j,$terms[$k],$ftnarg;
	    $args[$j] = $ftnarg;
	    $ftcargs[$j] = "*" . $ftcargs[$j];
	}
	$j++;
    }
    return join(', ',@ftcargs);
}

# $m = "struct timespec";
# $x = "if (clock_gettime(CLOCK_REALTIME,&local) != 0)
#        perror(\"clock_gettime\");"; 
# $zt = "unsigned long long";
# $hdrs = "";

$m = "long long";
$x = "PAPI_get_real_cyc()";
$zt = "long long";
$hdrs = "";
$syscalls = 0;
$ftn = 0;
$i = 0;

while ($i <= $#ARGV) {
    if ("$ARGV[0]" eq "-s") {
	$syscalls = 1;
    } elsif ("$ARGV[0]" eq "-f") {
	$ftn++;
    } 
    shift;
}

while (<>)
{
    chomp;
    if (/\(/)
    {
	if (/^(.*)\s+([_a-zA-Z0-9]+)\s*\((.*)\)$/) 
	{
	    push @LR,$1;
	    push @LN,$2;
	    push @LA,"($3)";
	    push @LAN,getargs("($3)");
	    push @FP,"(".make_fortran_proto($3).", int *rc)";
	    push @F2C,make_fortran_call2c($3);
	}
	else 
	{
	    print "Invalid line $_\n";
	    exit 1;
	}
    } else 
    {
	if ($z eq undef) 
	{ 
	    $z = "$_"; 
#	    $y = "\tglobal_$z += $x - localstart;";
	    $y = "\tif (thread != NULL && ${z}_gate && i_started_counter) { thread->$z += $x - localstart; ${z}_perthread_mutex = 0; }";
	} else 
	{
	    $hdrs .= "$_\n";
	}
    }
}

$hdrs .= "#include \"papi.h\"\n";
$hdrs .= "#include \"papiex_internal.h\"\n";

print "#include <stdio.h>\n#include <dlfcn.h>\n#include <time.h>\n#include <sys/types.h>\n";
print "#include <sys/syscall.h>\n" if ($syscalls);
printf $hdrs;
print "\nstatic void handle_any_dlerror();\n\n";

$i=0;
while ($i<=$#LR)
{
    $r = $LR[$i];
    $n = $LN[$i];
    $a = $LA[$i];
    $an = $LAN[$i];
    print "#define PARAMS_$n $a\n\n";
    print "typedef $r (*${n}_fptr_t) PARAMS_$n;\n\n";
    print "static ${n}_fptr_t real_$n = NULL;\n\n";
    $i++;
}

#print "extern __thread $zt global_${z};\n\n";
print "static int ${z}_gate = 0;\n\n";
print "static __thread int ${z}_perthread_mutex = 0;\n\n";

# Function to handle error
print "static void handle_any_dlerror()\n";
print "{\n";
    print "\tchar *err;\n";
    print "\tif ((err = dlerror()) != NULL)\n";
    print "\t{\n";
        printf "\t\tfprintf(stderr,\"libmonitor error: %%s \\n\", err); exit(1);\n";
    print "\t}\n";
print "}\n";

print "void ${z}_set_gate(int on)\n{\n";
print "\t${z}_gate = on;\n";
print "}\n\n";

print "void ${z}_init(void)\n{\n";
$i=0;
while ($i<=$#LR)
{
    $r = $LR[$i];
    $n = $LN[$i];
    $a = $LA[$i];
    $an = $LAN[$i];
    print "if (real_$n == NULL) { real_$n = (${n}_fptr_t)dlsym(RTLD_NEXT,\"$n\");\n";
    print "handle_any_dlerror();\n}\n";
    $i++;
}
print "}\n";

$i=0;
while ($i<=$#LR)
{
    $r = $LR[$i];
    $n = $LN[$i];
    $a = $LA[$i];
    $an = $LAN[$i];
print "$r $n PARAMS_$n\n";
print "{\n";
print "\tpapiex_perthread_data_t *thread = NULL;\n";
print "\tvoid *tmp = NULL;\n";
print "\t$m localstart = 0;\n";
#    print STDERR $r;
if ($r ne "void") {
    print "\t$r retval;\n";
    $re = "retval =";
    $rv = "retval;";
} else {
    $re = "";
    $rv = "";
}
print "\tint i_started_counter = 0;\n";
print "\tif (${z}_gate && !${z}_perthread_mutex)\n";
print "\t{\n";
print "\t\t${z}_perthread_mutex = 1;\n";
print "\t\ti_started_counter = 1;\n";
print "#ifdef HAVE_PAPI\n";
print "\t\tint ret = PAPI_get_thr_specific(PAPI_TLS_USER_LEVEL1, &tmp);\n";
print "\t\tif (ret != PAPI_OK)\n";
print "\t\t  LIBPAPIEX_PAPI_ERROR(\"PAPI_get_thr_specific\",ret);\n";
print "\t\tthread = (papiex_perthread_data_t *)tmp;\n";
print "#endif\n";
print "\t\tlocalstart = $x;\n";
print "\t}\n";
if ($syscalls) {
    print "
    if (!real_$n) {
	real_$n = (${n}_fptr_t)dlsym(RTLD_NEXT,\"$n\"); 
 	handle_any_dlerror();
	if (real_$n == NULL) {
#if defined(SYS_$n) || defined(__NR_$n)
    #if defined(SYS_$n)
    	$re syscall(SYS_$n,$an);
    #elif defined(__NR_$n)
    	$re syscall(__NR_$n,$an);
    #endif
#else
    LIBPAPIEX_ERROR(\"Symbol \%s could not be found.\\n\",\"$n\");
#endif
	}
     $re (*real_$n)($an);
    } else
    $re (*real_$n)($an);\n";
} else {
    print "
    if (!real_$n) {
	real_$n = (${n}_fptr_t)dlsym(RTLD_NEXT,\"$n\"); 
 	handle_any_dlerror();
	if (real_$n == NULL) {
		abort();
	}
	$re (*real_$n)($an);
	} else
	$re (*real_$n)($an);\n";
}
    print "$y\n\treturn $rv;\n}\n";
    $i++;
}

# Fortran, single underscore
if ($ftn >= 1) {
    $i=0;
    while ($i<=$#LR)
    {
	$r = $LR[$i];
	$n = $LN[$i];
	$ln = $n;
	$ln =~ tr/[A-Z]/[a-z]/;
	$a = $FP[$i];
	$an = $F2C[$i];
	print "void ${ln}_ $a\n";
	print "{ *rc = ${n}($an); return; }\n";
	$i++;
    }
}

# Fortran, double underscore
if ($ftn >= 2) {
    $i=0;
    while ($i<=$#LR)
    {
	$r = $LR[$i];
	$n = $LN[$i];
	$ln = $n;
	$ln =~ tr/[A-Z]/[a-z]/;
	$a = $FP[$i];
	$an = $F2C[$i];
	print "void ${ln}__ $a\n";
	print "{ *rc = ${n}($an); return; }\n";
	$i++;
    }
}
