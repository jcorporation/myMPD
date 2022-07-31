#!/usr/bin/perl -w
use strict;

my $basedir = $ARGV[0];
if (not defined($basedir)) {
    print "Usage: tojson.pl <basedir>";
    exit 1
}
my $phrases;
my $i18n;
my @langs = ();

#get translation files
opendir my $dir, "src/i18n/" or die "Can't open directory: $!";
while (my $entry = readdir $dir) {
    push @langs, $1 if $entry =~ /^(\w+\-\w+)\.txt$/;
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

#Write i18n.js
open my $i18nfile, ">$basedir/i18n.json" or die "Can not open $basedir/i18n.json: $!";
print $i18nfile "{\"locales\":[";
print $i18nfile "{\"code\":\"default\",\"desc\":\"Browser default\"},";
my $i = 0;
for my $lang (sort @langs) {
    if ($i > 0) {
        print $i18nfile ",";
    }
    open my $file, "src/i18n/".$lang.".txt" or die "Can't open ".$lang.".txt";
    my $desc = <$file>;
    chomp($desc);
    print $i18nfile "{\"code\":\"$lang\",\"desc\":\"$desc\"}";
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
print $i18nfile "],";

my %outdated;
#write translations files
for my $lang (@langs) {
    my $j = 0;
    open my $phrasesfile, ">$basedir/$lang.json" or die "Can not open $basedir/$lang.json: $!";
    print $phrasesfile "{";
    for my $key (sort keys %$phrases) {
        if (defined($i18n->{$key}->{$lang})) {
            print $phrasesfile "," if $j > 0;
            print $phrasesfile "\"$key\":\"".$i18n->{$key}->{$lang}."\"";
            $j++;
        }
        elsif ($lang ne "en-US") {
            warn "Phrase \"".$key."\" for ".$lang." not found\n";
            if (defined($outdated{$lang})) {
                $outdated{$lang}++;
            }
            else {
                $outdated{$lang} = 1;
            }
        }
    }
    print $phrasesfile "}";
    close($phrasesfile);
}

#print outdated translations
print $i18nfile "\"missingPhrases\":{";
$i = 0;
for my $key (keys %outdated) {
    if ($i > 0) {
        print $i18nfile ",";
    }
    print $i18nfile "\"".$key."\":".$outdated{$key};
    $i++;
}
print $i18nfile "}}\n";
close($i18nfile);

#check for obsolet translations
for my $key (sort keys %$i18n) {
    if (not defined($phrases->{$key})) {
        warn "Obsolet translation \"".$key."\" for lang ".join(", ", keys %{$i18n->{$key}})."\n";
    }
}
