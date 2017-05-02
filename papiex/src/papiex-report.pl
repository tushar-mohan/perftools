#!/usr/bin/perl -w -s

$::d ||= 0;
$::derived = 1 unless defined($::derived);
$::totals = 1 unless defined($::totals);
$::write =1 unless defined($::write);
$::PAD_LENGTH = 45;
$::nosearch ||= 0;
$::prefix="" unless defined($::prefix);

$::usage = "
`basename $0` [-nosearch] -spec=<spec file> [-output=<output file>] [-write=0] [-derived=0] [-totals=0] [-min=0] <directory>
`basename $0` [-nosearch] -spec=<spec file> [-output=<output file>] [-write=0] [-derived=0] [-totals=0] [-min=0] <file>...\n";

die "A specification file must be specified using -spec\n" unless ($::spec); chomp $::spec;

$SCRIPT_DIR=`dirname $0`; chomp $SCRIPT_DIR;
$SPECS_FILE = "${SCRIPT_DIR}/../share/papiex/$::spec";
$SPECS_FILE = "$::spec" if ($::nosearch);

( -f $SPECS_FILE ) or die "Could not open $SPECS_FILE for reading : $!\n";
print "Using specs file: $SPECS_FILE\n" if ($::d);

@exe_fields_ordered = (); # ordered list of exe-related fields
%global_data = ();
%process_data = ();
@event_descriptions_ordered = ();
%event_descriptions = ();
%labelmap = ();   #map label index to labels

$::multiplex = 0;   # we set this if multiplexing is enabled
$::quiet = 1;       # we set this if QUIET is enabled in papiex
$::mpx_interval = 10;
$::MPX_MIN_SAMPLES = 10;

$ntasks = 0;

# returns 1 if all args are same
sub same {
  my $val = shift(@_);
  for my $nval (@_) {
    return 0 if ($val != $nval);
  }
  return 1;
}

sub min {
  my $a = shift(@_); 
  my $b = shift(@_); 
  return $a if ($a <= $b);
  return $b;
}

sub max {
  my $a = shift(@_); 
  my $b = shift(@_); 
  return $a if ($a >= $b);
  return $b;
}

sub mean {
  my $prev_mean = shift(@_); 
  my $new_val = shift(@_); 
  my $new_count = shift(@_); 
  my $new_mean = (($prev_mean * ($new_count-1)) + $new_val)/$new_count;
  return $new_mean;
}

sub process_environment {
  return unless defined($ENV{'PAPIEX_REPORT_ARCH_VALUES'});
  my $var = $ENV{'PAPIEX_REPORT_ARCH_VALUES'}; chomp $var;
  print "Found PAPIEX_REPORT_ARCH_VALUES=\"$var\" in the environment.\n" if ($::d);
  my @pairs = split(/,/, $var);
  for $tuple (@pairs) {
    next unless ($tuple =~ /(\S+)=(.*)/ );
    print "\tSetting $1 => $2\n" if ($::d >= 5);
    local $SIG{__WARN__} = sub { die $_[0] };
    eval("\$$1 = $2 ;");
    if ($@) {
      die "Could not eval: \$$1 = $2;\n";
    }
  }
}

sub pretty_print {
  my $str = shift(@_);
	my $oref = shift(@_);
        $str = "  $str" if ($str =~ /^\[\d+\].*/); # indent calipers
	print $oref "$str ";
	for ($i=length($str); $i<$PAD_LENGTH; $i++) {
	  print $oref ".";
	}
}

sub print_labelmap {
  my $oref = shift(@_);
  return unless (keys %labelmap);
  print $oref "\nCaliper Labels:\n";
  for my $label_index (keys %labelmap) {
    printf $oref "  %-5s %s\n", "[$label_index]", $labelmap{$label_index};
  }
}

sub get_output_handle {
  defined($::output) and return OUTPUT;
  defined($::base_dir) or return STDOUT;
  $::write or return STDOUT;
  my $pid = shift(@_);
  my $tid = shift(@_);
  my $task_string = "";
  if (($pid ne "") || ($tid ne "")) {
    if ( -d "$::base_dir/$pid" ) {
      $task_string = "/$pid/" ;
    }
    if (($ntasks > 1) && ($pid ne "")) {
      $task_string = "/pid_$pid";
      $task_string = "/task_" . $process_data{$pid}{'RANK'} if defined($process_data{$pid}{'RANK'});
    }
    if ($tid ne "") {
      $tid = "/thread_$tid";
    }
  }

  if (($ntasks > 1) && ($tid ne "")) {
    system("mkdir -p \"$::base_dir$task_string\"");
  }
  my $path = $::base_dir . $task_string . $tid . ".report.txt";
  if (-f $path) {
    open(HANDLE, ">>$path") or die "Could not open $path for appending : $!\n";
  }
  else {
    open(HANDLE, ">$path") or die "Could not open $path for writing : $!\n";
  }
  $handle = HANDLE;
  return($handle);
}

sub compute_totals {
  print "Computing totals for the processes\n" if ($::d);
  for my $pid (keys %process_data) {
    next unless ($pid =~ /^\d+$/);
    my $nthreads = 0;
    for my $tid (keys %{$process_data{$pid}}) {
      next unless ($tid =~ /^\d+$/);
      $nthreads++;
      for my $field (keys %{$process_data{$pid}{$tid}{'counts'}}) {
        if (defined($process_data{$pid}{'counts'}{$field})) {
          $process_data{$pid}{'counts'}{$field} += $process_data{$pid}{$tid}{'counts'}{$field};
          $process_data{$pid}{'stats'}{$field}{'min'} = min($process_data{$pid}{'stats'}{$field}{'min'}, $process_data{$pid}{$tid}{'counts'}{$field});
          $process_data{$pid}{'stats'}{$field}{'max'} = max($process_data{$pid}{'stats'}{$field}{'max'}, $process_data{$pid}{$tid}{'counts'}{$field});
          $process_data{$pid}{'stats'}{$field}{'mean'} = mean($process_data{$pid}{'stats'}{$field}{'mean'}, $process_data{$pid}{$tid}{'counts'}{$field}, $nthreads);
	}
	else {
          $process_data{$pid}{'counts'}{$field} = $process_data{$pid}{$tid}{'counts'}{$field};
          $process_data{$pid}{'stats'}{$field}{'min'} = $process_data{$pid}{$tid}{'counts'}{$field};
          $process_data{$pid}{'stats'}{$field}{'max'} = $process_data{$pid}{$tid}{'counts'}{$field};
          $process_data{$pid}{'stats'}{$field}{'mean'} = $process_data{$pid}{$tid}{'counts'}{$field};
          $process_data{$pid}{'stats'}{$field}{'squared_diff'} = 0;
	}
      }
    }
    # compute standard deviation across threads
    if ($nthreads > 1) {
      for my $tid (keys %{$process_data{$pid}}) {
        next unless ($tid =~ /^\d+$/);
        for my $field (keys %{$process_data{$pid}{$tid}{'counts'}}) {
          if (defined($process_data{$pid}{'counts'}{$field}) && defined($process_data{$pid}{'stats'}{$field}{'mean'})) {
            $process_data{$pid}{'stats'}{$field}{'squared_diff'} += ($process_data{$pid}{'stats'}{$field}{'mean'} - $process_data{$pid}{$tid}{'counts'}{$field}) ** 2;
          }
        }
      }
    }
  }

  print "Computing job totals\n" if ($::d);
  $ntasks = 0;
  for my $pid (keys %process_data) {
    next unless ($pid =~ /^\d+$/);
    $ntasks++;
    for my $field (keys %{$process_data{$pid}{'counts'}}) {
      my $rank = 0;
      $rank = $process_data{$pid}{'RANK'} if (defined($process_data{$pid}{'RANK'}));
      $rank_data{'counts'}{$field}{$rank} = $process_data{$pid}{'counts'}{$field};
      if (defined($global_data{'counts'}{$field})) {
        $global_data{'counts'}{$field} += $process_data{$pid}{'counts'}{$field};
        $global_data{'stats'}{$field}{'min'} = min($global_data{'stats'}{$field}{'min'}, $process_data{$pid}{'counts'}{$field});
        $global_data{'stats'}{$field}{'max'} = max($global_data{'stats'}{$field}{'max'}, $process_data{$pid}{'counts'}{$field});
        $global_data{'stats'}{$field}{'mean'} = mean($global_data{'stats'}{$field}{'mean'}, $process_data{$pid}{'counts'}{$field}, $ntasks);
      }
      else {
        $global_data{'counts'}{$field} = $process_data{$pid}{'counts'}{$field};
        $global_data{'stats'}{$field}{'min'} = $process_data{$pid}{'counts'}{$field};
        $global_data{'stats'}{$field}{'max'} = $process_data{$pid}{'counts'}{$field};
        $global_data{'stats'}{$field}{'mean'} = $process_data{$pid}{'counts'}{$field};
        $global_data{'stats'}{$field}{'squared_diff'} = 0;
      }
    }
  }

  # compute standard deviation across tasks
  if ($ntasks > 1) {
    for my $pid (keys %process_data) {
      next unless ($pid =~ /^\d+$/);
      for my $field (keys %{$process_data{$pid}{'counts'}}) {
        if (defined($global_data{'counts'}{$field}) && defined($global_data{'stats'}{$field}{'mean'})) {
          $global_data{'stats'}{$field}{'squared_diff'} += ($global_data{'stats'}{$field}{'mean'} - $process_data{$pid}{'counts'}{$field}) ** 2;
        }
      }
    }
  }

  my $ntasks_field = "Num. of tasks";
  $global_data{$ntasks_field} = $ntasks;
  push(@exe_fields_ordered, $ntasks_field); # unless exists($global_data{$ntasks_field}); # add each field once
  # handle global wallclock usecs specially
  if (defined($::ENV{'PAPIEX_WALLCLOCK_USECS'})) {
    print "PAPIEX_WALLCLOCK_USECS=$::ENV{'PAPIEX_WALLCLOCK_USECS'} found in the environment\n" if ($::d);
    $global_data{'counts'}{'Wallclock usecs'} = $::ENV{'PAPIEX_WALLCLOCK_USECS'};
  }
}

sub compute_single_level_metrics {
  my $href = shift; 
  return unless defined ($href->{'counts'});
  for my $field (keys %{$href->{'counts'}}) {
     $nospace_field = $field;
     $nospace_field =~ s/ /_/g ;
     $$nospace_field = $href->{'counts'}{$field};
     print "\t\t$nospace_field <= ${$nospace_field}\n" if ($::d >= 5);
     defined (${$nospace_field}) or die "Could not set ${$nospace_field}\n";
  }
  @specs_ordered = ();
  @temp_lvals = (); # temporary lvalues we define . These have to be removed later
  SPECS_LINE:
  for $_ (@specs) {
    my $stmt = $_; chomp $stmt; 
    next SPECS_LINE if ($stmt =~ /^\s*#.*/); # skip comments
    my $lval;
    if (/^\s*\$(.+\S)\s*=/) {
		  # derived metric
      $lval = $1;
    }
    else {
      push(@specs_ordered, "TEXT:$stmt"); # this is just a plain text line we have to print in order
      next SPECS_LINE;
    }
    #push(@specs_ordered, $lval);
    print "\tEvaluating >>>$stmt<<<\n" if ($::d >= 5);
    local $SIG{__WARN__} = sub { die $_[0] };
    eval("$stmt");
    if ($@) {
      print "\t\tsome event was not measured; skipping $stmt\n" if ($::d >=5);
      next SPECS_LINE;
    }
    push(@specs_ordered, $lval);
    $href->{'derived'}{$lval} = ${$lval} unless ( ($lval =~ /::/) || ($lval =~ /tmp_/) ); # skip temporary and constants
    my $formatted_lval = $lval ; $formatted_lval =~ s/_/ /g;
    printf("%50s : %10.5g\n", $formatted_lval, ${$lval}) if (($::d >= 2) && defined($href->{'derived'}{$lval}));
    push(@temp_lvals, $lval) if ($lval =~ /tmp_/); 
    #undef ${$lval}; # clean up stuff added in the specs file apart from temporaries
  }

  # clean up the namespace for stuff we added on the top
  for my $field (keys %{$href->{'counts'}}) {
    $nospace_field = $field;
    $nospace_field =~ s/ /_/g ;
    undef $$nospace_field;
  }
  for my $tmp_lval (@temp_lvals) {
    undef $$tmp_lval;
  }
}

sub compute_metrics {
  open (SPECS, "< $SPECS_FILE") or die "Could not open $SPECS_FILE for reading : $!\n";
  @specs = <SPECS>;
  close SPECS;

  for my $pid (keys %process_data) {
    next unless ($pid =~ /^\d+$/) ;
    for my $tid (keys %{$process_data{$pid}}) {
      next unless ($tid =~ /^\d+$/) ;
      compute_single_level_metrics(\%{$process_data{$pid}{$tid}})
    }
    compute_single_level_metrics(\%{$process_data{$pid}})
  }
  compute_single_level_metrics(\%global_data);
  # Handle global Wallclock_seconds specially
  if (defined($::ENV{'PAPIEX_WALLCLOCK_USECS'})) {
    $global_data{'derived'}{'Wallclock_seconds'} = $::ENV{'PAPIEX_WALLCLOCK_USECS'}/1000000; 
  }
}

sub print_mpx_warning {
  return unless ($::multiplex);
  my $href = shift(@_);
  my $oref = shift(@_);
  return unless defined($href->{'counts'}{'Virtual usecs'});
  if ($href->{'counts'}{'Virtual usecs'} <= (1000000 * $::MPX_MIN_SAMPLES)/$::mpx_interval) {
    print $oref "\n--- WARNING ---\n" .
                "The number of samples are too few, so the numbers below are suspect.\n" .
                "You can either run without multiplexing (without -a and -m) or increase\n" .
                "the multiplex frequency, from the current value of " . $::mpx_interval . " using -m<freq>\n\n";
  }
}

sub print_single_level {
  my $type = shift(@_);  # type is "counts" or "derived"
  my $href = shift(@_);
  my $out_ref = shift(@_);
  defined ($href->{$type}) or return;
  print_mpx_warning($href, $out_ref) if ($::multiplex);
  my @fields;
  if ($type eq "counts") {
    @fields = sort keys %{$href->{$type}};
  }
  else {
    @fields = @specs_ordered;
  }
  #for my $field (sort keys %{$href->{$type}}) {
  for my $field (@fields) {
    if ($field =~ /^TEXT:/) {
      my $plain_text = $field;
      $plain_text =~ s/^TEXT:// ;
      # plain text line, with no variables has to be simply printed
      print $out_ref "$plain_text\n";
      next ;
    }
    next unless defined($href->{$type}{$field});
    my $formatted_field = $field;
    my $percent_field = 0;
    $percent_field = 1 if  ($formatted_field =~ m/Percent/gi);
    $formatted_field =~ s/_/ /g if ($type eq "derived");
    $formatted_field =~ s/Percent/\%/gi if ($type eq "derived");
    #printf $out_ref "%50s : %12g\n", $formatted_field, $href->{$type}{$field};
    pretty_print($formatted_field, $out_ref);
    $href->{$type}{$field} = 100 if ($percent_field && ($href->{$type}{$field} > 100));
    printf $out_ref " %12.5e", $href->{$type}{$field} if (!defined($::min) || ($href->{$type}{$field} >= 0));
    printf $out_ref " %12.5e", 0  if (defined($::min) && ($href->{$type}{$field} < 0));
    if (defined($href->{'stats'}{$field}) 
        && (keys %{$href->{'stats'}{$field}})
        && (!same($href->{'stats'}{$field}{'min'},$href->{'stats'}{$field}{'max'},$href->{$type}{$field}))) {
      #  && ($href->{$type}{$field} != 0)) {
      printf $out_ref " %12.5e", $href->{'stats'}{$field}{'min'};
      printf $out_ref " %12.5e", $href->{'stats'}{$field}{'max'};
      printf $out_ref " %12.5e", $href->{'stats'}{$field}{'mean'};
      #printf $out_ref " %12.5e", sqrt($href->{'stats'}{$field}{'squared_diff'}/$ntasks);
      printf $out_ref " %12.5e", (sqrt($href->{'stats'}{$field}{'squared_diff'}/$ntasks)/$href->{'stats'}{$field}{'mean'});
    }

    # caliper: print % of caliper value w.r.to thread (or process) value
    if (($field =~ /^\[\d+\] (.*)$/) && ($href->{$type}{$field} >  0)) {
      my $base_field = $1;
      if (defined($href->{$type}{$base_field}) && ($href->{$type}{$base_field} > 0)) {
        printf $out_ref " [%5.1f]", ((100 * $href->{$type}{$field})/$href->{$type}{$base_field});
      }
    }

    printf $out_ref "\n";
    #printf $out_ref "%50s : %12g\n", $formatted_field, $href->{$type}{$field};
  }
  print_labelmap($out_ref) if ($type eq "counts");
  print_event_descriptions($out_ref) if ($type eq "counts");
  close($out_ref) unless (ref($out_ref) == ref(STDOUT));
}

sub check_stale_reports {
  if (defined($::base_dir)) {
	  my @old_files;
    ( -f "$::base_dir.report.txt" ) && push (@old_files, "$::base_dir.report.txt");
	  @old_files = `find $::base_dir -name \"*.report.txt\"`;
		die "The directory $::base_dir already contains reports:\n @old_files\n" .
		    "Please remove them and try again\n" if (@old_files);
	}
}

sub print_event_descriptions {
  my $oref = shift(@_);
  return unless  (%event_descriptions) ;  # only works for non-tied hashes
  print $oref "\nEvent Descriptions:\n";
  for my $event (@event_descriptions_ordered) {
    printf $oref "%-30s: %s\n", $event, $event_descriptions{$event};
  } 
}

sub print_process_header {
  my $oref = shift(@_);
  my $pid = shift(@_);
  my @pids = ();
  @pids = (keys %process_data) unless defined($pid);
  push(@pids, $pid) if (defined($pid));
  for my $p (@pids) {
    printf $oref "%-30s: %s\n", "Num. of threads", $process_data{$p}{'NTHREADS'} if (defined( $process_data{$p}{'NTHREADS'}));
  }
}

sub print_data {
  my $type = shift(@_);  # type is "counts" or "derived"
	my $oref;
  for my $pid (keys %process_data) {
    next unless ($pid =~ /^\d+$/) ;
                my $nthreads = 0;
		if (defined($process_data{$pid}{'NTHREADS'}) && ($process_data{$pid}{'NTHREADS'} > 1)) {
      			for my $tid (keys %{$process_data{$pid}}) {
        			next unless ($tid =~ /^\d+$/) ;
                                $nthreads++;
			 	$oref = get_output_handle($pid, $tid);
        			print $oref "Thread (PID $pid, TID $tid) $type data:\n";
      				print_single_level($type,  \%{$process_data{$pid}{$tid}}, $oref);
      			}
		}
		$oref = get_output_handle($pid, "");
		my $rank_string = "";
		my $thread_string = "";
		$rank_string = ", MPI Rank $process_data{$pid}{'RANK'}" 
                    if (($global_data{'NPROCS'} > 1) && defined( $process_data{$pid}{'RANK'}));
		$thread_string = ", $process_data{$pid}{'NTHREADS'} threads" if (defined( $process_data{$pid}{'NTHREADS'}));
      		print $oref "Process (PID $pid$rank_string$thread_string) $type data:\n";
                printf $oref "%-${PAD_LENGTH}s   %-12s %-12s %-12s %-12s %-12s\n","Event","Sum","Min","Max","Mean","CV"
                  if (($nthreads>1) && ($type eq "counts"));
  		print_single_level($type, \%{$process_data{$pid}}, $oref);
  }
  if ($global_data{'NPROCS'} > 1) {
    $oref = get_output_handle("", "");
    print_process_header($oref) if (($global_data{'NPROCS'} == 1) && ($type eq "derived"));
    print $oref "\nGlobal $type data:\n";
    printf $oref "%-${PAD_LENGTH}s   %-12s %-12s %-12s %-12s %-12s\n","Event","Sum","Min","Max","Mean","CV"
      if (($global_data{'NPROCS'} > 1) && ($type eq "counts"));
    print_single_level($type, \%global_data, $oref);
  }
}

sub dump_specs {
  my $specsfile = shift(@_);
  my $oref = get_output_handle("", "");
  if (-f $specsfile) {
    my @specs = `cat $specsfile|sed 's/[\$:;{}#]//g'`;
    local $" = "";
    print $oref "\nDerived Metric Descriptions:\n@specs\n";
  }
}


sub print_rankmap {
  my $oref = get_output_handle("", "");
  print $oref "\nRank mapping:\n";
  for my $rank (sort keys %rankmap) {
    print $oref "[$rank] => " . $rankmap{$rank}{'host'} . " (PID " . $rankmap{$rank}{'pid'} . ")\n";
  }
}

sub print_rank_data {
  my $type = shift(@_);
  my $oref = get_output_handle("", "");
  print $oref "\nRank $type data (by field):";
  for my $field (sort keys %{$rank_data{$type}}) {
    print $oref "\n$field\n";
    for my $rank (sort { $rank_data{$type}{$field}{$b} <=> $rank_data{$type}{$field}{$a} } keys %{$rank_data{$type}{$field}}) {
      printf $oref "%12.5e [%d]\n", $rank_data{$type}{$field}{$rank}, $rank;
    }
  }
}


sub print_exe_info {
  my $href = shift(@_);
  my $oref = get_output_handle("", "");
	for my $field (@exe_fields_ordered) {
	  next if (($field eq "derived") || ($field eq "counts"));
		printf $oref "%-30s: %s\n", $field, $href->{$field};
	}
}

sub process_input {
  my $pid = undef;
  my $tid = undef;
  my $label_index = 0;

	if (-d $ARGV[0]) {
	  defined ($ARGV[1]) && die $::usage;
	  $::base_dir = shift(@ARGV); chomp $::base_dir; $::base_dir =~ s|/*$|| ; # remove trailing slashes
		my $cmd="find $::base_dir -name \"*_[0-9]*.txt\"";
		@ARGV = `$cmd`;
  	}

	INPUT_LINE:
	while (<>) {
		my $field; my $val;
		# exe info (global data)
		#if (/^(\S.*\S)\s+:\s*(\S.*$)/) {
		if (/^([A-Za-z][a-z].+\S)\s+:\s*(\S.*$)/) {
			$field = $1; 
			$val = $2; chomp $val;

			if ($field =~ /^Hostname/i) {
				$process_host = $val;
			}
			if ($field =~ /^Process id/i) {
				$pid = $val; $pid =~ s/\s+//g;
                                $label_index = 0;
				undef $tid; # reset thread id
			}
			if ($field =~ /^Mpi Rank/i) {
				$val =~ s/\s+//g;
				$process_data{$pid}{'RANK'} = $val;
                                $rankmap{$val}{'pid'} = $pid;
                                $rankmap{$val}{'host'} = $process_host || "";
				next INPUT_LINE;
			}
			if ($field =~ /^Thread id/i) {
				$tid = $val; $tid =~ s/\s+//g;
				if (defined($process_data{$pid}{'NTHREADS'})) {
				  $process_data{$pid}{'NTHREADS'}++;
				}
				else {
				  $process_data{$pid}{'NTHREADS'} = 1;
				}
				next INPUT_LINE;
			}
			if ($field =~ /^Clockrate|Processor/i) {
				$$field = $val; $$field =~ s/\s+//g;
				$::clockmhz = $val; # specially handle mhz
				chomp($::clockmhz);
			}
			if ($field =~ /^Options/i) {
				$::quiet = 1 if ($val =~/QUIET/);
				$::mpx_interval = $1 if ($val =~/MPX_INTERVAL=(\d+)/);
			}
			my $org_field = $field;
			$field =~ s/\s+/_/g ;
			if (defined($global_data{$field}) && ($global_data{$field} ne $val)) {
				$global_data{$field} = "NOT_UNIQUE";
			}
			else {
				push(@exe_fields_ordered, $org_field) unless exists($global_data{$org_field}); # add each field once
				$global_data{$org_field} = $val;
			}
			printf("%30s: %30s\n", $org_field, $global_data{$org_field}) if ($::d >= 2);
		}

		# LABEL (put this clause before the event count value pair clause
		elsif (/^([0-9.e]+)\s+\[LABEL\] (.*)$/) {
			$label_index = $1; 
			my $label = $2; chomp $label;
                        die "Invalid value ($label_index) for label index for $label. Permissible values are >= 1\n" 
                          if ($label_index < 1);
			die "<$_> cannot parsed in a context where PID is not set\n" unless defined($pid);
                        $labelmap{$label_index} = $label;
			printf "\n%35s  %5d     [%6d(0x%x)]\n","[LABEL] $label",$label_index,$pid,$tid if($::d >= 2);
		}

		# event counts value pair 
		elsif (/^([0-9.e]+)\s+(\S.*\S)$/) {
			$val = $1; 
			$field = $2; chomp $field;
                        #$field = "[$label_index] $field" if ($label_index > 0); # caliper

			die "<$_> cannot parsed in a context where PID is not set\n" unless defined($pid);
			if (($field =~ s/^\[PROCESS\] //i) || !defined($tid)) {
				# process stats, not per-thread stats
				$process_data{$pid}{'counts'}{$field} = $val;
				printf("%30s: %10.5e     [%6d]\n", $field, $val, $pid) if ($::d >= 2);
			}
			else {
				$process_data{$pid}{$tid}{'counts'}{$field} = $val;
				printf("%30s: %10.5e     [%6d(0x%x)]\n", $field, $val, $pid, $tid) if ($::d >= 2);
			}
		}

		# event descriptions
		elsif (/^(\S+)\s*: (\S.+)$/) {
			my $event = $1;
			my $description = $2;
			if (!defined($event_descriptions{$event})) {
				$event_descriptions{$event} = $description;
				push(@event_descriptions_ordered, $event);
			}
		}

		# for backward compatibility, tuples in the old format
		#elsif (/^(\w[^\.%]+) \.{5,}\s*([0-9.e]+)$/) {
		#	$val = $2;  chomp $val;
		#	$field = $1; chomp $field;
		#	die "<$_> cannot parsed in a context where PID is not set\n" unless defined($pid);
		#	if (($field =~ s/^\[PROCESS\] //i) || !defined($tid)) {
		#		# process stats, not per-thread stats
		#		$process_data{$pid}{'counts'}{$field} = $val;
		#		printf("%30s: %12g     [%6d]\n", $field, $val, $pid) if ($::d >= 2);
		#	}
		#	else {
		#		$process_data{$pid}{$tid}{'counts'}{$field} = $val;
		#		printf("%30s: %12g     [%6d(0x%x)]\n", $field, $val, $pid, $tid) if ($::d >= 2);
		#	}
		#}

		EOF_CHECK:
		if (eof()) {
			undef $pid;
			undef $tid;
		}
	}
	$global_data{'NPROCS'} = keys %process_data;
	$ntasks = $global_data{'NPROCS'};

	# handle global Start and Finish specially
        if (defined ($::ENV{'PAPIEX_WALLCLOCK_START'})) {
	  $global_data{'Start'} = $::ENV{'PAPIEX_WALLCLOCK_START'}; chomp $global_data{'Start'};
          print "PAPIEX_WALLCLOCK_START=\"$global_data{'Start'}\" found in the environment\n" if ($::d);
        }
        if (defined ($::ENV{'PAPIEX_WALLCLOCK_FINISH'})) {
	  $global_data{'Finish'}=$::ENV{'PAPIEX_WALLCLOCK_FINISH'}; chomp $global_data{'Finish'};
          print "PAPIEX_WALLCLOCK_FINISH=\"$global_data{'Finish'}\" found in the environment\n" if ($::d);
        }

	print "Finished reading papiex output files\n" if ($::d);
	if (! defined($::base_dir)) {
	  my $exe = `basename $global_data{'Executable'}` ; chomp $exe;
	  #defined($::output) or $::output = $::prefix . $exe . ".papiex." . $global_data{'NPROCS'} . "." . $::host . ".".  $::outpid . "." . $::instance .  ".report.txt";
	  defined($::output) or $::output = $::prefix . $exe . ".papiex." . $::host . ".".  $::outpid . "." . $::instance .  ".report.txt";
	}

	if (defined($::output)) {
		open(OUTPUT, ">$::output") or die "Could not open $::output for writing: $!\n";
  	}
}

# main
$::outpid = $$;
$::host = `hostname`; chomp $host;
$::instance = 1;
if ($ARGV[0] =~ /.*\.papiex\.(\S+)\.(\d+)\.(\d+)/) {
  $::host = $1;
  $::outpid = $2;
  $::instance = $3;
}
check_stale_reports();
process_environment();
process_input();
compute_totals() if ($::totals);
print_exe_info(\%global_data);
if ($::derived) {
  compute_metrics();
  print_data('derived');
}
print_data('counts');
print_rankmap() if ($ntasks > 1);
print_rank_data('counts') if ($ntasks > 1);
dump_specs($SPECS_FILE);
defined(OUTPUT) and close(OUTPUT);
if (!$::quiet) {
  if (defined($::output) && $::output) {
    print "papiex: Output summary in [$::output]\n";
  }
  elsif ($::write && $::base_dir) {
    print "\npapiex: Per-thread and per-process reports and raw counts in $::base_dir/\n";
    print "papiex: Job summary report in $::base_dir" . ".report.txt\n";
  }
}
