#!/usr/bin/perl -w
use strict;

my $pretty = defined($ARGV[0]) ? ($ARGV[0] eq "pretty" ? 1 : 0) : 0;
my $phrases;
my $i18n;
my @langs = ();

#get translation files
opendir my $dir, "." or die "Can't open directory: $!";
while (my $entry = readdir $dir) {
    push @langs, $1 if $entry =~ /^(\w+\-\w+)\.txt$/;
}
closedir $dir;

#extra phrases
open my $file, "extra_phrases.txt" or die "Can't open file \"extra_phrases.txt\": $!";
while (my $line = <$file>) {
    chomp($line);
    $phrases->{$line}->{"en-US"} = $line;
}
close $file;

#phrases from src
my @dirs = ("../", "../mpd_client/", "../mympd_api/", "../web_server/", "../../htdocs/js/");
my @files = ("../../htdocs/index.html");
for my $dirname (@dirs) {
    opendir my $dir, $dirname or die "Can't open directory \"$dirname\": $!";
    while (my $entry = readdir $dir) {
        next if $entry eq "bootstrap-native-v4.js";
        next if $entry eq "i18n.js";
        push @files, $dirname.$1 if $entry =~ /^(\w+\.(c|js))$/;
    }
    closedir $dir;
}

for my $filename (@files) {
    open my $file, $filename or die "Can't open file \"$filename\": $!";
    while (my $line = <$file>) {
        if ($filename =~ /\.c$/) {
            while ($line =~ /(jsonrpc_respond_message|jsonrpc_start_phrase)\([\w\->()]+\s*,\s*[\w\->]+\s*,\s*[\w\->]+,\s*"([^"]+)"/g) {
                $phrases->{$2} = 1;
            }
            while ($line =~ /(jsonrpc_start_phrase_notify)\([\w\-()]+\s*,\s*"([^"]+)"/g) {
                $phrases->{$2} = 1;
            }
        }
        elsif ($filename =~ /\.js$/) {
            while ($line =~ /(\s+|\(|\+)t\('([^']+)'/g) {
                $phrases->{$2} = 1;
            }
            while ($line =~ /gtPage\('([^']+)'/g) {
                $phrases->{$1} = 1;
            }
            while ($line =~ /"desc":\s*"([^"]+)"/g) {
                $phrases->{$1} = 1;
            }
        }
        elsif ($filename =~ /\.html$/) {
            while ($line =~ /data-(\w+-)?phrase="([^"]+)"/g) {
                $phrases->{$2} = 1;
            }
        }
    }
    close $file;
}

#print i18n.js
print "var locales=[";
print "\n\t" if $pretty eq 1;
my $i = 0;
for my $lang (sort @langs) {
    if ($i > 0) {
        print ",";
        print "\n\t" if $pretty eq 1;
    }
    open my $file, $lang.".txt" or die "Can't open ".$lang.".txt";
    my $desc = <$file>;
    chomp($desc);
    print "{\"code\":\"$lang\",\"desc\":\"$desc\"}";
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

print "\n" if $pretty eq 1;
print "];";
print "\n" if $pretty eq 1;
print "var phrases={";

$i = 0;
for my $key (sort keys %$phrases) {
    print "," if $i > 0;
    print "\n\t" if $pretty eq 1;
    print "\"".$key."\":{";
    print "\n" if $pretty eq 1;
    my $j = 0;
    for my $lang (sort @langs) {
        if (defined($i18n->{$key}->{$lang})) {
            print "," if $j > 0;
            print "\n" if $j > 0 and $pretty eq 1;
            print "\t\t" if $pretty eq 1;
            print "\"$lang\":\"".$i18n->{$key}->{$lang}."\"";
            $j++;
        }
        elsif ($lang ne "en-US") {
            warn "Phrase \"".$key."\" for ".$lang." not found\n";
        }
    }
    print "\n\t" if $pretty eq 1;
    print "}";
    $i++;
}
print "\n" if $pretty eq 1;
print "};\n";

#check for obsolet translations
for my $key (sort keys %$i18n) {
    if (not defined($phrases->{$key})) {
        warn "Obsolet translation \"".$key."\" for lang ".join(", ", keys %{$i18n->{$key}})."\n";
    }
}
