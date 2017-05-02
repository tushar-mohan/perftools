#!/usr/bin/perl -w -s

my $test_cnt = 0;
my $err = 0;

$help ||= 0; ${-help} ||=0; $h ||=0; ${-h} ||=0;

if ($help || ${-help} || $h || ${-h} || !@ARGV) {
  print STDERR "usage: test.pl [-debug] test1 ...\n";
  exit(1);
}

TEST: for $base (@ARGV) {

  $run_out = "$base.out";

  print "Processing $base\n" if ($debug);
  $output = `cat $run_out` || die "Could not open $run_out for reading: $!\n" ;
  @regex  = split /\n/, `cat $base.regex` || die "Could not open $base.regex for reading: $!\n" ;

REGEX:  for $exp (@regex) {
    chomp($exp);
    next REGEX if ($exp =~ m/^\s+$/); # skip blank lines
    next REGEX if ($exp =~ m/^#.*$/); # skip commented lines
    print "\tSearching for $exp .. " if ($debug);
    $test_cnt++;
    my $output_dup = $output;
    if ($output_dup =~ /$exp/mg) {
      print "found\n" if ($debug);
      next REGEX;
    }
    else {
      print "FAILED\n" if ($debug);
      print STDERR "\t$base failed: $exp could not be found\n";
      $err++;
      next REGEX;
    }
  }
}

my $passed = $test_cnt - $err;
print STDERR "Tests : $test_cnt\n";
print STDERR "Passed: $passed\n";
print STDERR "Failed:  $err\n";

exit($err);
