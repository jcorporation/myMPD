#!/usr/bin/perl -w
#
# Extracts the phrases from source, compare it to already translated phrases
# writes the i18n.json and reports the differences

use strict;

my $verbose = defined($ARGV[0]) ? 1 : 0;

my $phrases;
my $i18n;
my $desc;
my @langs = ();

#get translation files
opendir my $dir, "src/i18n/json" or die "Can't open directory \"src/i18n/json\": $!";
while (my $entry = readdir $dir) {
    push @langs, $1 if $entry =~ /^(\w+\-\w+)\.json$/;
}
closedir $dir;

#extra phrases
open my $file, "src/i18n/extra_phrases.txt" or die "Can't open file \"src/i18n/extra_phrases.txt\": $!";
while (my $line = <$file>) {
    chomp($line);
    $phrases->{$line}->{"en-US"} = $line;
}
close $file;

#phrases from src
my @dirs = ("src/", "src/mpd_client/", "src/mpd_worker/", "src/mympd_api/", "src/web_server/", "htdocs/js/");
my @files = ("htdocs/index.html");
for my $dirname (@dirs) {
    opendir my $dir, $dirname or die "Can't open directory \"$dirname\": $!";
    while (my $entry = readdir $dir) {
        next if $entry eq "bootstrap-native.js";
        next if $entry eq "long-press-event.js";
        next if $entry eq "i18n.js";
        next if $entry eq "apidoc.js";
        push @files, $dirname.$1 if $entry =~ /^(\w+\.(c|js))$/;
    }
    closedir $dir;
}

for my $filename (@files) {
    open my $file, $filename or die "Can't open file \"$filename\": $!";
    while (my $line = <$file>) {
        if ($filename =~ /\.c$/) {
            while ($line =~ /JSONRPC_SEVERITY_\w+,\s+"([^"]+)"(\)|,)/g) {
                $phrases->{$1} = 1;
            }
            while ($line =~ /JSONRPC_SEVERITY_\w+,\s+\S+,\s+"([^"]+)"(\)|,)/g) {
                $phrases->{$1} = 1;
            }
        }
        elsif ($filename =~ /\.js$/) {
            while ($line =~ /(\s+|\(|\+)tn?\('([^']+)'/g) {
                $phrases->{$2} = 1;
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

#Read translations
my $i = 0;
for my $lang (sort @langs) {
    open my $file, "src/i18n/json/".$lang.".json" or die "Can't open file \"".$lang.".json\": $!";
    while (my $line = <$file>) {
        if ($line =~ /^\s+\"(.+)\":\s+\"(.+)\",?$/) {
            $i18n->{$1}->{$lang} = $2;
        }
    }
    close $file;
    $i++;
}

#count missing phrases
my %outdated;
#write translations files
for my $lang (@langs) {
    $outdated{$lang} = 0;
    for my $key (sort keys %$phrases) {
        if (not defined($i18n->{$key}->{$lang}) and
            $lang ne "en-US")
        {
            warn "Phrase \"".$key."\" for ".$lang." not found\n" if $verbose eq 1;
            $outdated{$lang}++;
        }
    }
}

#read language descriptions
open my $descfile, "src/i18n/i18n.txt"  or die "Can not open src/i18n/i18n.txt: $!";
while (my $line = <$descfile>) {
    if ($line =~ /^(\w+-\w+):(.*)$/) {
        $desc->{$1} = $2;
    }
}
close $descfile;

#Write i18n.json
open my $docfile, ">docs/references/translating_status.md" or die "Can not open \"docs/references/translating_status.md\": $!";
open my $i18nfile, ">src/i18n/json/i18n.json" or die "Can not open \"src/i18n/json/i18n.json\": $!";
print $i18nfile "{\n";
print $i18nfile "    \"default\": {\"desc\":\"Browser default\", \"missingPhrases\": 0},\n";
$i = 0;
for my $lang (sort @langs) {
    if ($lang ne "de-DE" and
        $lang ne "en-US")
    {
        if ($outdated{$lang} > 0) {
            print $docfile "- $lang: $outdated{$lang} missing phrases\n";
        }
        else {
            print $docfile "- $lang: fully translated\n";
        }
    }
    if ($outdated{$lang} > 100) {
        warn "$lang: skipped, too many missing phrases (".$outdated{$lang}.")\n";
        next;
    }
    if ($i > 0) {
        print $i18nfile ",\n";
    }
    print $i18nfile "    \"".$lang."\": {\"desc\":\"".$desc->{$lang}."\", \"missingPhrases\": ".$outdated{$lang}."}";
    if ($outdated{$lang} > 0) {
        print STDERR "$lang: $outdated{$lang} missing phrases\n";
    }
    else {
        print STDERR "$lang: fully translated\n";
    }
    $i++;
}
print $i18nfile "\n}\n";
close $i18nfile;
close $docfile;

#check for obsolet translations
for my $key (sort keys %$i18n) {
    if (not defined($phrases->{$key})) {
        warn "Obsolet translation \"".$key."\" for lang ".join(", ", keys %{$i18n->{$key}})."\n" if $verbose eq 1;
    }
}
