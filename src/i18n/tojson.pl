#!/usr/bin/perl -w
#
# Reads the .txt files and writes the json translation files
#
use strict;

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

#Read translations
my $i = 0;
for my $lang (sort @langs) {
    open my $file, "src/i18n/".$lang.".txt" or die "Can't open ".$lang.".txt";
    #skip desc
    my $desc = <$file>;
    while (my $key = <$file>) {
        next if $key =~ /^\s*$/;
        chomp($key);
        my $value = <$file>;
        chomp($value);
        $value =~ s/\"/\\\"/g; #escape
        $key =~ s/\"/\\\"/g;   #escape
        $i18n->{$key}->{$lang} = $value;
    }
    close $file;
    $i++;
}

#count missing phrases
my %outdated;
#write translations files
for my $lang (@langs) {
    my $j = 0;
    $outdated{$lang} = 0;
    open my $phrasesfile, ">src/i18n/json/$lang.json" or die "Can not open src/i18n/json/$lang.json: $!";
    print $phrasesfile "{\n";
    for my $key (sort keys %$phrases) {
        if (defined($i18n->{$key}->{$lang})) {
            print $phrasesfile ",\n" if $j > 0;
            print $phrasesfile "    \"$key\": \"".$i18n->{$key}->{$lang}."\"";
            $j++;
        }
        elsif ($lang ne "en-US") {
            warn "Phrase \"".$key."\" for ".$lang." not found\n";
            $outdated{$lang}++;
        }
    }
    print $phrasesfile "\n}\n";
    close($phrasesfile);
}

#Write i18n.json
open my $i18nfile, ">src/i18n/json/i18n.json" or die "Can not open src/i18n/json/i18n.json: $!";
print $i18nfile "{\n    \"locales\": [\n";
print $i18nfile "        {\"code\":\"default\", \"desc\":\"Browser default\", \"missingPhrases\": 0},\n";
$i = 0;
for my $lang (sort @langs) {
    if ($outdated{$lang} > 100) {
        warn "Skipping $lang, too many missing phrases (".$outdated{$lang}.")\n";
        next;
    }
    if ($i > 0) {
        print $i18nfile ",\n";
    }
    open my $file, "src/i18n/".$lang.".txt" or die "Can't open ".$lang.".txt";
    my $desc = <$file>;
    chomp($desc);
    close $file;
    print $i18nfile "        {\"code\":\"$lang\", \"desc\":\"$desc\", \"missingPhrases\": ".$outdated{$lang}."}";
    $i++;
}
print $i18nfile "\n    ]\n}\n";
close($i18nfile);

#check for obsolet translations
for my $key (sort keys %$i18n) {
    if (not defined($phrases->{$key})) {
        warn "Obsolet translation \"".$key."\" for lang ".join(", ", keys %{$i18n->{$key}})."\n";
    }
}
