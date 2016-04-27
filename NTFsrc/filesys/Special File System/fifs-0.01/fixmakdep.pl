#!perl -w

#
# Copyright (c) 1998 by Danilo Almeida
# 
# This script fixes up a MSVC++ generated makefile so that it does
# not have absolute pathnames when they are not necessary.
#
# This script comes with no warranties of any kind.  Use it at your
# own risk.  It is made available under the terms of the GNU GPL v2.
#

use strict;

use IO::Handle;
use IO::File;

$0 =~ s/(.*[\/\\]([^\/\\]+))|(.*([^\/\\]+))/$2?$2:$4/e;

my @list;

if ($ARGV[0] && !($ARGV[0] eq '-d')) {
    @list = @ARGV;
} elsif ($ARGV[0] && ($ARGV[0] eq '-d')) {
    @list = ('filesys/fsmunge/fsmunge.mak',
	     'filesys/fsnfs/fsnfs.mak',
	     'filesys/fswin32/fswin32.mak',
	     'main/helpdll/fshelper.mak',
	     'main/helplib/helplib.mak',
	     'main/netapi32/netapi32.mak',
	     'main/server/server.mak',
	     'test/fst/fst.mak',
	     'test/nbt/nbt.mak');
} else {
    &usage();
    exit(0);
}

sub usage
{
    print "Usage: $0 [ -d | files... ]\n";
    print "   -d use default file list\n";
}

use Cwd;
use File::Basename;

my $start = cwd;

while(my $file = shift @list) {
    if (-T $file) {

	my ($name, $path) = fileparse($file);
	die if !chdir($path);
	$file = $name;

	my @dir = split(/[\\\/]/, cwd);

	my $drive = '';
	$drive = lc shift @dir if ($dir[0] && $dir[0] =~ /^.\:$/);

#	my $expr = "($drive)?";
#	foreach my $d (@dir) {
#	    $expr .= '[\\\/]'.$d;
#	}
#	print "$expr\n";
#	$expr = '^\s+((cd)|(chdir))\s+(\")?'.$expr.'[^\"]+(\")?';
#	print "$expr\n";

	my $expr2 = '^(\s+((cd)|(chdir))\s+(\"))?(.:)?[\\\/]([^\"]+)(\"\s*)?$$';

        print "$file\n";
        my $temp = "$file.~$0~";
	die "$temp already exists\n" if (-e $temp);
	my $fhi = new IO::File;
	my $fho = new IO::File;
	die "Cannot open $file\n" if !$fhi->open($file, O_RDONLY);
	die "Cannot open $temp\n" if !$fho->open($temp, O_CREAT | O_WRONLY);
	my $changed = 0;
        while (<$fhi>) {
	    if (/$expr2/i && !($drive && $6 && !($drive eq lc($6)))) {
		#print $_;
		my @odir = split (/[\\\/]/, $7);
		#print join("|", @dir);
		my $i;
		for ($i = 0; $i <= $#dir && $i <= $#odir; $i++) {
		    last if !($dir[$i] eq $odir[$i]);
		}
		if ($i) {
		    my $ndir = "..\\"x($#dir - $i + 1) . join("\\", @odir[$i..$#odir]);
		    print "\tChanging: $_";
 		    $_ = $1.$ndir.$8;
		    print "\t      To: $_";
		    $changed = 1;
		}
	    }
	    $fho->print($_);
	}
        $fhi->close();
        $fho->close();
	if ($changed) {
	    my ($dev,$ino,$mode,$nlink,$uid,$gid,$rdev,$size,
		$atime,$mtime,$ctime,$blksize,$blocks) = stat($file);
#	    utime $atime, $mtime, $temp;
	    rename($temp, $file);
	} else {
	    unlink($temp);
	}
    } else {
	print "Skipping $file\n";
    }
    die if !chdir($start);
}
