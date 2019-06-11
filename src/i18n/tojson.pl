#!/usr/bin/perl -w
use strict;

my $i18n;
my @langs = ("en-US", "de-DE");

my $p = defined($ARGV[0]) ? 1 : 0;

for my $lang (@langs) {
    open my $file, $lang.".txt" or die "Can't open ".$lang.".txt";
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
}

print "var phrases={";

my $i = 0;
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
