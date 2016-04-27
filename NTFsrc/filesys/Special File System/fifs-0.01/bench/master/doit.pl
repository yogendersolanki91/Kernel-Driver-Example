use strict;

$| = 1;

my($computer) = $ENV{"COMPUTERNAME"};
my($baseconf) = "\\Registry\\HKEY_LOCAL_MACHINE\\SOFTWARE\\etc\\fifs\\test";
my($bench_cmd) = "mb.pl";
my($log_loc) = "c:\\testing\\log";
my($bench_base) = "u:\\dalmeida\\src\\fifs\\bench";
my($local_path) = "c:\\testing";
my($largefile) = $bench_base."\\lfs\\largefile\\Release\\largefile.exe";
my($smallfile) = $bench_base."\\lfs\\ideal\\Release\\ideal.exe";
my($mb_base) = $bench_base."\\master";

my($SHARE_LETTER)  = "w:";
my($KNFS_LETTER)   = "x:";
my($KNFSNC_LETTER) = "y:";
my($SUBST_LETTER)  = "g:";

my(%TEST) = ();
my(%TEST2) = ();

$TEST{"local"}  = 0;
$TEST{"knfs"}   = 0;
$TEST{"knfsnc"} = 0;
$TEST{"W32-1"}  = 1;
$TEST{"W32-4"}  = 1;
$TEST{"W32M-1"} = 0;
$TEST{"W32M-4"} = 0;
$TEST{"NFS-1"}  = 0;
$TEST{"NFS-4"}  = 0;

$TEST2{"largefile"} = 5;
$TEST2{"smallfile"} = 5;
$TEST2{"mix"}       = 0;
$TEST2{"copy"}      = 0;

sub init {
    chdir $local_path;
#    system("subst g: $local_path");
    system("cmd /c md $log_loc");
    &copy($largefile, ".");
    &copy($smallfile, ".");
}

my(@others) =
    (
     "local",  $SUBST_LETTER."\\fsroot\\",
     "knfs",   $KNFS_LETTER."\\",
     "knfsnc", $KNFSNC_LETTER."\\",
     );

my(@fifs) = 
    (
     "W32-1",
     "W32-4",
     "W32M-1",
     "W32M-4",
     "NFS-1",
     "NFS-4"
     );

my($TIME) = 'gtime -f %Eelapsed';
my($TS) = "gdate";

my(%MB) = ();

$MB{"mix"}  = &init_mix();
$MB{"copy"} = &init_copy(1024, 16384);

sub init_mix {
    my(@cmds) =
        (
#         "Copy large file",               "cp 8_megabytes 8mb",
         "Copy source archive",           "cp test.zip t.zip",
         "unzip source archive",          "unzip t.zip",
         "Copy source tree",              "xcopy /e /i test test2",
         "du source tree",                "du -b test",
         "Diffing source trees",          "diff -r test test2",
         "Compile source tree",           "cmd /c \"cd test\\fifs\\main\\server & set CFG=server - Win32 Release& nmake /f server.mak\"",
         "du built tree",                 "du -b test",
         "Diffing built tree",            "diff -r test test2",
         "zip built tree",                "zip -r built.zip test\\*",
         "zip built tree as update",      "zip -r t.zip test\\*",
         "Remove unbuilt tree",           "cmd /c \"rd /s /q test2\"",
         "Remove built tree",             "cmd /c \"rd /s /q test\"",
         "Remove source archive",         "cmd /c \"del test.zip\"",
#         "Remove large file",             "cmd /c \"del 8_megabytes\"",
         "Total Time (Mix)",              0,
         );

    my(@files) = ("8_megabytes", "test.zip");
    my(@save) = ("built.zip", "8mb", "t.zip");

    my(@all) = (\@cmds, \@files, \@save);
    return \@all;
}

sub init_copy {
    my($max_copy_kb, $max_file_kb) = @_;

    my(@cmds) = 
        (
         "unzip size files",              "unzip size.zip",
         "Total Time (Unzip)",            0,
         );

    my($bits) = log($max_file_kb)/log(2);
    my($i);
    foreach $i (0..$bits) {
        my($file_kb) = 2**$i;
        my($num_copies) = &max(int($max_copy_kb/$file_kb), 1);
        my($copy_kb) = $num_copies * $file_kb;
        push(@cmds,
             (sprintf("copy %dkb (%dx -> %dkb)", 
                      $file_kb, $num_copies, $copy_kb), 
              sprintf("cmd /c \"for /l %%a in (1,1,%d) do ".
                      "\@copy size\\%dkb size\\%dkb.%%a > nul\"", 
                      $num_copies, $file_kb, $file_kb)));
        push(@cmds,
             (sprintf("du %dkb copies", $file_kb),
              sprintf("cmd /c \"du -cb size/%dkb.* | tail -1\"", $file_kb)));
        push(@cmds,
             (sprintf("delete %dkb copies", $file_kb),
              sprintf("cmd /c \"for /l %%a in (1,1,%d) do ".
                      "\@del size\\%dkb.%%a \"", $num_copies, $file_kb)));
    }

    push(@cmds,
         ("Total Time (Copy)",             0,
          "zip size files",                "zip -r s.zip size\\*",
          "Remove size files",             "cmd /c \"rd /s /q size\"",
          "Total Time (Zip/Cleanup)",      0,
          ));

    my(@files) = ("size.zip");
    my(@save) = ("s.zip");

    my(@all) = (\@cmds, \@files, \@save);
    return \@all;
}

sub main {
    my($i);
    for $i (0..($#others/2)) {
        &do_other($others[2*$i],
                  $others[2*$i+1]);
    }
    foreach $i (@fifs) {
        &do_fifs($i, $SHARE_LETTER);
    }
}

sub copy {
    my($in, $out) = @_;
    print "Copying $in to $out\n";
    system("cmd /c copy $in $out");
}

sub test {
    my($base, $name, $context, $pre, $post) = @_;
    my($path) = $base.$name;
    return if !$TEST{$name};
    my($log) = $log_loc."\\".$name;
    &test_path($path, $log, $context, $pre, $post) if ($TEST{$name});
}

sub do_other {
    my($name, $base) = @_;
    &test($base, $name, 0, 0, 0);
}

sub do_fifs {
    my($conf, $drive) = @_;
    return if !$TEST{$conf};
    my($regconf) = $baseconf."\\".$conf;
    my($share) = "\\\\".$computer."-".$conf."\\tree";
    my(@context) = ($regconf, $drive, $share);
    &fifs_pre(@context);
    &test($drive."\\", $conf, 0, 0, 0);
    &fifs_post(@context);
}

sub test_path {
    my($path, $log, $context, $pre, $post) = @_;
    system($TS);
    print "---- TESTING $path -> $log ----\n";
    system("cmd /c \"md $log\"");
    return if $pre && &$pre(@$context);
    system("cmd /c \"md $path\"");
    &$post(@$context) if $post;
    &do_largefile("largefile", 0,    $path, $log, $context, $pre, $post);
    &do_smallfile("smallfile", 0,    $path, $log, $context, $pre, $post);
    &do_mb       ("mix",       \%MB, $path, $log, $context, $pre, $post);
    &do_mb       ("copy",      \%MB, $path, $log, $context, $pre, $post);
}

sub do_largefile {
    my($type, $info, $path, $log, $context, $pre, $post) = @_;
    my($iterations) = $TEST2{$type};
    return if !$iterations;
    print "RUNNING $type ($iterations times)\n";

    &$pre(@$context) if $pre;
    system("cmd /c \"md $path\"");

    $log = "$log\\lf";
    print STDERR "Logging to file: $log\n";
    open(SAVEOUT, ">&STDOUT");
    if (open(STDOUT,">$log") == 0) {
	print STDERR "Error: Could not write to logfile: $log\n";
	return;
    }

    my($i);
    foreach $i (1..$iterations) {
        system("largefile -f 8 $path");
    }
    foreach $i (1..$iterations) {
        system("largefile -f 8 -i 8 $path");
    }
    &$post(@$context) if $post;

    close(STDOUT);
    open(STDOUT, ">&SAVEOUT");
}

sub do_smallfile {
    my($type, $info, $path, $log, $context, $pre, $post) = @_;
    my($iterations) = $TEST2{$type};
    return if !$iterations;
    print "RUNNING $type ($iterations times)\n";

    &$pre(@$context) if $pre;
    system("cmd /c \"md $path\"");

    $log = "$log\\sf";
    print STDERR "Logging to file: $log\n";
    open(SAVEOUT, ">&STDOUT");
    if (open(STDOUT,">$log") == 0) {
	print STDERR "Error: Could not write to logfile: $log\n";
	return;
    }

    my(@cmds) = 
        (
         "create"    , "./ideal $path 1 1000 10 100 1",
         "read"      , "./ideal $path 1 1000 10 100 3",
         "write"     , "./ideal $path 1 1000 10 100 4",
         "write/sync", "./ideal $path 1 1000 10 100 5",
         "delete"    , "./ideal $path 1 1000 10 100 2",
         );
    my($num) = $#cmds / 2;

    system($TS);
    my($i);
    foreach $i (1..$iterations) {
        my($j);
        foreach $j (0..$num) {
            my($banner) = sprintf("%12s (%d): %s\n", $cmds[2*$j], $i,
                                  $cmds[2*$j+1]);
            print STDERR "$banner\n";
            print "$banner\n";
            system($TIME." ".$cmds[2*$j+1]." 2>&1");
            system($TS);
        }
    }
    &$post(@$context) if $post;

    close(STDOUT);
    open(STDOUT, ">&SAVEOUT");

}

sub fifs_pre {
    my($regconf, $drive, $share)= @_;
    return -1 if system("cmd /c start /min server $regconf");
    if (system("net use $drive $share")) {
        system("kill -f server");
        return -1;
    }
    system("cmd /c vol $drive");
    return 0;
}

sub fifs_post {
    my($regconf, $drive, $share)= @_;
    system("net use $drive /d");
    system("kill server");
}

sub do_mb {
    my($type, $info, $path, $log, $context, $pre, $post) = @_;
    my($iterations) = $TEST2{$type};
    return if !$iterations;
    print "RUNNING $type ($iterations times)\n";

    $info = $$info{$type};
    my($benchinfo, $files, $save) = @$info;

    my(@benchinfo) = @$benchinfo;
    push(@benchinfo,
         (
          "Final Time",                    -1
          ));

    my(%bench) = ();
    my($i);

    &$pre(@$context) if $pre;
    &$post(@$context) if $post;

    print STDERR "Doing $iterations iterations\n";

    $log = "$log\\$type";
    print STDERR "Logging to file: $log\n";
    open(SAVEOUT, ">&STDOUT");
    if (open(STDOUT,">$log") == 0) {
	print STDERR "Error: Could not write to logfile: $log\n";
	return;
    }

    print STDERR "Starting.\n";


    my($max) = $#benchinfo / 2;
    my(@names, @commands);
    foreach $i (0..$max) {
        $names[$i] = $benchinfo[2*$i];
        $commands[$i] = $benchinfo[2*$i+1];
    }

    my($pass) = 0;
    system($TS);
    while ($pass++ < $iterations) {
        print STDERR "Pass: $pass\n";
        print "\#\n\# Doing pass $pass\n\#\n";
        &$pre(@$context) if $pre;
        system("cmd /c \"md $path\"");
        foreach $i (@$files) {
            &copy($mb_base."\\".$i, $path."\\.");
        }
        chdir $path;
        my($lsum) = 0;
        my($tsum) = 0;
        foreach $i (0..$max) {
            if ($commands[$i] && !($commands[$i] < 0)) {
                $lsum += $bench{$i,$pass} = &run($names[$i], $commands[$i]);
            } else {
                if (!$commands[$i]) {
                    $bench{$i,$pass} = $lsum;
                    $tsum += $lsum;
                } else {
                    $bench{$i,$pass} = $tsum;
                }
                $lsum = 0;
                printf "%s: %3.2f seconds\n", $names[$i], $bench{$i,$pass};
            }
            printf STDERR "%s: %3.2f seconds\n", $names[$i], $bench{$i,$pass};
        }
        system($TS);
        chdir $local_path;
        foreach $i (@$save) {
            &copy($path."\\".$i, $log.".$i.$pass");
        }
        system("cmd /c \"rd /s /q $path\"");
        &$post(@$context) if $post;
    }

    foreach $i (0..$max) {
        my(@numbers) = ();
        my($j);
        foreach $j (1..$iterations) {push(@numbers,$bench{$i,$j});}
        printf "%-28s  -  min %3.2f max %3.2f ",
        @names[$i],&minimum(@numbers),&maximum(@numbers);
        my($avg) = &average(@numbers);
        printf "avg %3.2f  stddev: %3.2f\n",$avg,&stddev($avg,@numbers);
    }

    print "\nTimes for each run\n";
    foreach $i (0..$max) {
        my(@numbers) = ();
        my($j);
        foreach $j (1..$iterations) {push(@numbers,$bench{$i,$j});}
        printf "times %-28s: ",@names[$i];
        foreach $i (@numbers) {printf "%3.2f ",$i;}	
        print "\n";
    }
    close(STDOUT);
    open(STDOUT, ">&SAVEOUT");

}

#
# Subroutines
#

sub run {
    my($name, $cmd) = @_;
    my(@times);

    $| = 1;

    print "Running: $cmd\n<<\n";

    open(TIME, $TIME." $cmd 2>&1 | ") ||
        die("Can't run time $cmd: $!\n");

    while(<TIME>) {
        if(/^(\d+):(\d+\.\d+)elapsed/) {
            push(@times, $1 * 60 + $2);
        } else {
            print "$_";
        }
    }
    close(TIME);
    print ">>\n";
    $#times > -1 || die "There were no times!\n";

    foreach (@times) {
        print "$name: <$cmd> took ";
        printf "%.2f  CPU seconds\n", $_;
        return $_;
    }
}

sub minimum {
    my(@numbers) = @_;
    my($min, $i);
    $min = @numbers[0];
    foreach $i (@numbers) { $min = $i if ($i < $min);}
    return $min;
}

sub maximum {
    my(@numbers) = @_;
    my($max, $i);
    $max = @numbers[0];
    foreach $i (@numbers) { $max = $i if ($i > $max);}
    return $max;
}

sub average {
    my($size,$sum,$i);
    my(@numbers) = @_;
    $size = @numbers;
    $sum = 0;
    foreach $i (@numbers) {$sum += $i;}
    return $sum/($#numbers + 1);
}


sub stddev {
    my($avg,@numbers) = @_;
    my($sum,$i);
    $sum = 0;
    foreach $i (@numbers) {$sum += ($i - $avg)**2;}
    return sqrt $sum ;

}

sub max { my($a, $b) = @_; return $a>$b?$a:$b }

&init();
&main();
