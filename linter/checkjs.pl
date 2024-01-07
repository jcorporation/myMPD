#!/usr/bin/perl -w
#
# SPDX-License-Identifier: GPL-3.0-or-later
# myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
# https://github.com/jcorporation/mympd
#

use strict;

my $used_cmds;
my $functions;
my $methodes;
my @files = ();
my $file;

#get all cmds from html
open $file, "htdocs/index.html" or die "Can not open \"htdocs/index.html\": $!";
while (my $line = <$file>) {
    while ($line =~ s/data-href='\{"cmd":\s*"([^"]+)"//g) {
        $used_cmds->{$1} = "index.html";
    }
}

#get all javascript files
my $dirname = "htdocs/js/";
opendir my $dir, $dirname or die "Can't open directory \"$dirname\": $!";
while (my $entry = readdir $dir) {
    next if $entry eq "bootstrap-native.js";
    next if $entry eq "long-press-event.js";
    next if $entry eq "i18n.js";
    next if $entry eq "apidoc.js";
    push @files, $dirname.$1 if $entry =~ /^([\w-]+\.js)$/;
}
closedir $dir;

#get all cmds from javascript
for my $filename (@files) {
    open $file, $filename or die "Can't open file \"$filename\": $!";
    while (my $line = <$file>) {
        while ($line =~ s/"cmd": "([^"]+)"//g) {
            $used_cmds->{$1} = $filename;
        }
        if ($line =~ /function ([^)]+)\(/) {
            $functions->{$1} = 1;
        }
    }
    close($file);
}

open $file, "src/lib/api.h" or die "Can't open file \"src/lib/api.h\": $!";
while (my $line = <$file>) {
    if ($line =~ /^\s+X\(([^)]+)\)/) {
        $methodes->{$1} = 1;
    }
}
close $file;

#check if all used cmds are defined
my $rc = 0;
for my $cmd (keys %$used_cmds) {
    if ($cmd =~ /^MYMPD_API/) {
        if (not defined($methodes->{$cmd})) {
            print "Method $cmd not defined: $used_cmds->{$cmd}\n";
        }
    }
    elsif (not defined($functions->{$cmd})) {
        print "Function $cmd not defined: $used_cmds->{$cmd}\n";
        $rc=1;
    }
}

exit $rc;
