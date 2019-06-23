#!/usr/bin/perl -w
use strict;

my $p = defined($ARGV[0]) ? 1 : 0;
my $i18n;
my @langs = ();

opendir my $dir, "." or die "Can't open directory: $!";
while (my $entry = readdir $dir) {
    push @langs, $1 if $entry =~ /^(\w+\-\w+)\.txt$/;
}
closedir $dir;

print "var locales=[";
my $i = 0;
for my $lang (sort @langs) {
    print "," if $i > 0;
    open my $file, $lang.".txt" or die "Can't open ".$lang.".txt";
    my $desc = <$file>;
    chomp($desc);
    print "{\"code\": \"$lang\",\"desc\": \"$desc\"}";
    while (my $key = <$file>) {
        next if $key =~ /^\s*$/;
        chomp($key);
        my $value = <$file>;
        chomp($value);
        $value =~ s/\"/\\\"/g;
        $key =~ s/\"/\\\"/g;
        $i18n->{$key}->{$lang} = $value;
    }
    close $file;
    $i++;
}

print "];";
print "\n" if $p eq 1;
print "var phrases={";

$i = 0;
for my $key (sort keys %$i18n) {
    print "," if $i > 0;
    print "\n\t" if $p eq 1;
    print "\"".$key."\":{";
    print "\n" if $p eq 1;
    my $j = 0;
    for my $lang (sort keys %{$i18n->{$key}}) {
        print "," if $j > 0;
        print "\n" if $j > 0 and $p eq 1;
        print "\t\t" if $p eq 1;
        print "\"$lang\":\"".$i18n->{$key}->{$lang}."\"";
        $j++;
    }
    print "\n\t" if $p eq 1;
    print "}";
    $i++;
}

print "};";
print "\n" if $p eq 1;
