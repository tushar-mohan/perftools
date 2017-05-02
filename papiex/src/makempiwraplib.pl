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
    my $didsomething = 0;
    my $skipptr = 0;
    while ($j <= $#args) {
	$args[$j] = trim($args[$j]);
#    printf "processing arg |%s|\n",$args[$j];
	my @terms = split ' ',$args[$j];
	my $k = $#terms;
	if ($k != 1) {
	    print "Error: There should only be 2 terms vs $k for $args[$j]\n";
	    exit 1;
	}
	if ($terms[0] =~ /^MPI_Status.*/) {
	    if ($terms[$k] =~ /^\*(array.*)/) {
		$skipptr = 1;
		$terms[$k] = $1 . "[][sizeof(MPI_Status)/sizeof(MPI_Fint)]"; }
	    $didsomething = 1;
	}
	if ($terms[0] =~ /^MPI_.*/) {
	    $terms[0] = "MPI_Fint";
	    $didsomething = 1;
	}
	if (!($terms[$k] =~ /^\*.*/)) {
	    if (!$skipptr) {
		$terms[$k] = "*" . $terms[$k]; }
	    $didsomething = 1;
	}
	if ($didsomething) {
	    $skipptr = 0;
	    my $ftnarg = join(' ', @terms);
#	    printf "old arg |%s|, new arg |%s|\n",$args[$j],$ftnarg;
	    $args[$j] = $ftnarg;
	    $didsomething = 0;
	}
	$j++;
    }
    return join(', ',@args);
}

sub make_fortran_prolog
{
    my $input = shift;
    my @args = split ',',$input;
    my $j = 0;
    my $didsomething = 0;
    my @decls;

    push @decls,"\tint rc;";
    while ($j <= $#args) {
	$args[$j] = trim($args[$j]);
#    printf "processing arg |%s|\n",$args[$j];
	my @terms = split ' ',$args[$j];
	my $k = $#terms;
	my $tmp = "";
	if ($k != 1) {
	    print "Error: There should only be 2 terms vs $k for $args[$j]\n";
	    exit 1;
	}
	if (!($terms[$k] =~ /^\*.*/)) {
	    $tmp = "*";
	}
	if ($terms[0] =~ /^MPI_Datatype/) {
	    push @decls, "\tMPI_Datatype c_" . $terms[$k] . " = MPI_Type_f2c(" . $tmp . $terms[$k] . ");";
	    $didsomething = 1;
	}
	if ($terms[0] =~ /^MPI_Comm/) {
	    push @decls, "\tMPI_Comm c_" . $terms[$k] . " = MPI_Comm_f2c(" . $tmp . $terms[$k] . ");";
	    $didsomething = 1;
	}
	if ($terms[0] =~ /^MPI_Op/) {
	    push @decls, "\tMPI_Op c_" . $terms[$k] . " = MPI_Op_f2c(" . $tmp . $terms[$k] . ");";
	    $didsomething = 1;
	}
	if ($terms[0] =~ /^MPI_Status/) {
	    $terms[$k] =~ /^\*(.*)/;
	    my $name = $1;
	    if ($name =~ /^array.*/) {
		my $tmp = "\tMPI_Status *c_" . $name . " = (MPI_Status*)malloc(sizeof(MPI_Status)*(*count));\n";
		$tmp = $tmp . "\tif (c_$name == NULL) {\n\tLIBPAPIEX_ERROR(\"Failed to allocate memory for %d MPI_Status's\",*count); }\n";
		push @decls, $tmp;
		$didsomething = 1;
	    }
	    else {
		push @decls, "\tMPI_Status c_" . $name . ";";
		$didsomething = 1;
	    }
	}
	if ($terms[0] =~ /^MPI_Request/) {
	    if ($terms[$k] =~ /^\*(in_.*)/) {
		my $name = $1;
		if ($name =~ /^in_array/) {
		    my $tmp = "\tMPI_Request *c_" . $name . " = (MPI_Request*)malloc(sizeof(MPI_Request)*(*count));\n";
		    $tmp = $tmp . "\tif (c_$name == NULL) {\n\tLIBPAPIEX_ERROR(\"Failed to allocate memory for %d MPI_Request's\",*count); }\n";
		    $tmp = $tmp . "\t{ int i; for (i=0;i<*count;i++) {\n\t\tc_$name";
		    $tmp = $tmp . "[i] = MPI_Request_f2c($name";
		    $tmp = $tmp . "[i]); } }";
		    push @decls, $tmp;
		    $didsomething = 1;
		}
		else {
		    push @decls, "\tMPI_Request c_" . $name . " = MPI_Request_f2c(" . $tmp . $terms[$k] . ");";
		    $didsomething = 1;
		}
	    }
	    elsif ($terms[$k] =~ /^\*(out_.*)/) {
		my $name = $1;
		push @decls, "\tMPI_Request c_" . $name . ";";
		$didsomething = 1;
	    } else {
		print "Error: MPI_Request parameters should have in_ or out_ prefix on arguments in wrapper spec.\n";
		exit 1;
	    }
	}
	$j++;
    }
    return join("\n",@decls);
}

sub make_fortran_epilog
{
    my $input = shift;
    my @args = split ',',$input;
    my $j = 0;
    my $didsomething = 0;
    my @epilog;

    while ($j <= $#args) {
	$args[$j] = trim($args[$j]);
#    printf "processing arg |%s|\n",$args[$j];
	my @terms = split ' ',$args[$j];
	my $k = $#terms;
	my $tmp = "";
	if ($k != 1) {
	    print "Error: There should only be 2 terms vs $k for $args[$j]\n";
	    exit 1;
	}
	if ($terms[0] =~ /^MPI_Status/) {
	    $terms[$k] =~ /^\*(.*)/;
	    my $name = $1;
	    $tmp = $tmp . "\tif (rc == MPI_SUCCESS)\n";
	    if ($name =~ /^array/) {
		$tmp = $tmp . "\t{ int i; for (i=0;i<*count;i++) {\n\t\t";
		$tmp = $tmp . "MPI_Status_c2f(&c_$name" . "[i], &" . $name . "[i][0]); } }\n";
		$tmp = $tmp . "\tfree(c_$name);";
		push @epilog, $tmp;
		$didsomething = 1;
	    } else {
		$tmp = $tmp . "\tMPI_Status_c2f(&c_" . $name . ", " . $name . ");";
		push @epilog, $tmp;
		$didsomething = 1;
	    }
	} elsif ($terms[0] =~ /^MPI_Request/) {
	    if ($terms[$k] =~ /^\*(in_.*)/) {
		my $name = $1;
		if ($name =~ /^in_array/) {
		    $tmp = $tmp . "\t{ int i; for (i=0;i<*count;i++) {\n\t\t$name";
		    $tmp = $tmp . "[i] = MPI_Request_c2f(c_$name";
		    $tmp = $tmp . "[i]); } }\n";
		    $tmp = $tmp . "\tfree(c_$name);";
		    push @epilog, $tmp;
		    $didsomething = 1;
		} else {
		    push @epilog, "\t*" . $name . " = MPI_Request_c2f(c_" . $name . ");";
		    $didsomething = 1;
		}
	    } elsif ($terms[$k] =~ /^\*(out_.*)/) {
		my $name = $1;
		push @epilog, "\t*" . $name . " = MPI_Request_c2f(c_" . $name . ");";
		$didsomething = 1;
	    }
	}
	$j++;
    }

    push @epilog, "\t*ierr = (MPI_Fint)rc;";
    return join("\n",@epilog);
}

sub make_fortran_call2c
{
    my $input = shift;
    my @args = split ',',$input;
    my $j = 0;
    my @ftcargs = split ',',getargs($input);
    my $didsomething = 0;
#    print @ftcargs, "\n";
    while ($j <= $#args) {
	$args[$j] = trim($args[$j]);
#    printf "processing arg |%s|\n",$args[$j];
	my @terms = split ' ',$args[$j];
	my $k = $#terms;
#	print "$terms[0]\n";
	if (($terms[0] =~ /^MPI_Datatype/) || ($terms[0] =~ /^MPI_Comm/) || ($terms[0] =~ /^MPI_Op/)) {
	    $terms[$k] = "c_" . $terms[$k];
#	    print "YES $terms[$k]\n";
	    $didsomething = 1;
	}
	elsif (($terms[0] =~ /^MPI_Request/) && ($terms[$k] =~ /^\*(.*)/)) {
	    my $tmp = $1;
	    if (($tmp =~ /^in_array.*/) || ($tmp =~ /^out_array.*/)) {
		$terms[$k] = "c_" . $tmp;
	    } else {
		$terms[$k] = "&c_" . $tmp;
	    }
	    $didsomething = 1;
	}
	elsif (($terms[0] =~ /^MPI_Status/) && ($terms[$k] =~ /^\*(.*)/)) {
	    my $tmp = $1;
	    if ($tmp =~ /^array.*/) {
		$terms[$k] = "c_" . $tmp;
	    } else {
		$terms[$k] = "&c_" . $tmp;
	    }
	    $didsomething = 1;
	}
	elsif (!($terms[$k] =~ /^\*.*/)) {
#	    $ftnptrarg[$j] = 1; 
	    $terms[$k] = "*" . $terms[$k];
	    $didsomething = 1;
	}
	if ($didsomething) {
#	    my $ftnarg = join(' ', @terms);
#	    printf "new arg |%s|\n",;
	    $ftcargs[$j] = $terms[$k];
	    $didsomething = 0;
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
    if (/^#/)
	{
	    $hdrs .= "$_\n";
	}
    elsif (/\(/)
    {
#	print "open paren",$_;
	if (/^(.*)\s+([_a-zA-Z0-9]+)\s*\((.*)\)$/) 
	{
#	    print "match",$_;
	    push @LR,$1;
	    push @LN,$2;
	    push @LA,"($3)";
	    push @LAN,getargs("($3)");
	    push @FP,"(".make_fortran_proto($3).", MPI_Fint *ierr)";
	    push @F2C,make_fortran_call2c($3);
	    push @FD,make_fortran_prolog($3);
	    push @FE,make_fortran_epilog($3);
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
	    $y = "\tif (thread != NULL && ${z}_gate) thread->$z += $x - localstart;";
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
print "\t$r retval;\n";
print "\tif (${z}_gate)\n";
print "\t{\n";
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
    	retval = syscall(SYS_$n,$an);
    #elif defined(__NR_$n)
    	retval = syscall(__NR_$n,$an);
    #endif
#else
    LIBPAPIEX_ERROR(\"Symbol \%s could not be found.\\n\",\"$n\");
#endif
	} 
    retval = (*real_$n)($an);
    } else
    retval = (*real_$n)($an);\n";
} else {
    print "
    if (!real_$n) {
	real_$n = (${n}_fptr_t)dlsym(RTLD_NEXT,\"$n\"); 
 	handle_any_dlerror();
	if (real_$n == NULL) {
		abort();
	}
	retval = (*real_$n)($an);
	} else
	retval = (*real_$n)($an);\n";
}
    print "$y\n\treturn retval;\n}\n";
    $i++;
}

# Fortran, default is double underscore
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
	if ((${ln} =~ /^mpi_waitsome/) || (${ln} =~ /^mpi_waitany/)) {
	    $i++;
	    next;
	}
	print "void ${ln}__ $a\n";
	print "{\n";
	print "$FD[$i]\n";
	print "\trc = ${n}($an);\n";
	print "$FE[$i]\n";
	print "}\n";
	print "#pragma weak ${ln} = ${ln}__\n";
	print "#pragma weak ${ln}_ = ${ln}__\n";
	$un = $ln;
	$un =~ tr/[a-z]/[A-Z]/;
	print "#pragma weak ${un} = ${ln}__\n";
	print "#pragma weak ${un}_ = ${ln}__\n";
	print "#pragma weak ${un}__ = ${ln}__\n";
	$i++;
    }
}
