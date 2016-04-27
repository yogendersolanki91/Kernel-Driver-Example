use strict;

my($dir) = shift(@ARGV);
#my($min) = shift(@ARGV);
#my($max) = shift(@ARGV);

&usage() if !$dir;

my($max_size) = 16384;

my($data) = " " x 1024;
my($size) = 1;
my($file);
while ($size <= $max_size) {
    $file = "$dir/$size"."kb";
    die "could not open $file\n" if !open(OUT, ">$file");
    print "Writing $file...";
    print OUT $data;
    close(OUT);
    print "done\n";
    $data x= 2;
    $size *= 2;
}

sub usage {
    die "usage: $0 directory\n";
}
