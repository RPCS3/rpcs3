#!/usr/bin/perl

use strict;
use warnings;
use Data::Dumper;

# find in perl is a PITA so let's do it easy for the moment
#
# find ./locales -name "pcsx2_Main.po" -exec ./linux_various/check_po_quality.pl {} \;

my $main = $ARGV[0];
my $ico = $main;
$ico =~ s/Main/Iconized/;

open(my $MAIN, "<$main") or die "failed to open $main";
print "\nCheck $main\n";
check_c_format($MAIN);

print "Check $ico\n";
open(my $ICO1, "<$ico") or die "failed to open $ico";
check_double_key($ICO1);
open(my $ICO2, "<$ico") or die "failed to open $ico";
check_c_format($ICO2);

sub check_double_key {
    my $file = shift;
    my $line;
    my $index;
    while ($line = <$file>) {
        if ($line =~ /^#~/) {
            next;
        }
        if ($line =~ /^#:\s*(.*)/) {
            $index = $1; # help for debug
        }
        if ($line =~ /"!/) {
            print "$index\n";
            print "Warning translation beginning with '!'\nEnsure it is not the old double key translation\n\n";
            die;
        }
    }
}

sub check_c_format {
    my $file = shift;

    my $line;
    my $index;

    my $in_c_format;
    my $in_msgid;
    my $in_msgstr;
    my $is_empty;

    my @c_symbol;

    while ($line = <$file>) {
        if ($line =~ /^#~/) {
            next;
        }
        if ($line =~ /^#:\s*(.*)/) {
            my $old_index = $index;
            $index = $1; # help for debug
            
            if (scalar(@c_symbol) > 0 and not $is_empty) {
                print "$old_index\n";
                print "Error: translation miss some c format\n";
                print Dumper @c_symbol;
                print "\n";
                die;
            }
            my @empty;
            @c_symbol = @empty;

            $in_c_format = 0;
            $in_msgid = 0;
            $in_msgstr = 0;
            $is_empty = 1;
        }
        if ($line =~ /^#.*c-format/) {
            $in_c_format = 1;
        }
        if ($line =~ /^\s*msgid\s/ and $in_c_format) {
            $in_msgid = 1;
            $in_msgstr = 0;
        }
        if ($line =~ /^\s*msgstr\s/ and $in_c_format) {
            $in_msgid = 0;
            $in_msgstr = 1;
        }
        if ($in_msgid and $line =~ /%\w/) {
            my @symbols = $line =~ /%\w/g;
            #print "add symbol @symbols\n";
            push(@c_symbol, @symbols);
        }
        if ($in_msgstr and $line =~ /".+"/) {
            $is_empty = 0;
        }
        if ($in_msgstr and $line =~ /%\w/) {
            my @symbols = $line =~ /%\w/g;
            #print "get symbol @symbols\n";
            foreach my $symbol (@symbols) {
                if ($c_symbol[0] ne $symbol) {
                    print "$index\n";
                    print "Error: C format mismatch\n\n";
                    die;
                }
                shift @c_symbol;
            }
        }
    }
}
