#!/usr/bin/perl

# PCSX2 - PS2 Emulator for PCs
# Copyright (C) 2002-2011  PCSX2 Dev Team
#
# PCSX2 is free software: you can redistribute it and/or modify it under the terms
# of the GNU Lesser General Public License as published by the Free Software Found-
# ation, either version 3 of the License, or (at your option) any later version.
#
# PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
# without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
# PURPOSE.  See the GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along with PCSX2.
# If not, see <http://www.gnu.org/licenses/>.

# This basic script convert an jpeg/png image to a .h include file 
# compatible with PCSX2 gui

use File::Basename;
use strict;
use warnings;

sub ascii_to_hex ($)
{
    (my $str = shift) =~ s/(.|\n)/sprintf("%02lx", ord $1)/eg;
    return $str;
}

sub get_wx_ext ($)
{
    my $ext = shift;

    my $result = "BAD_FORMAT";
    if ($ext =~ /png/i) {
        $result = "wxBITMAP_TYPE_PNG";
    } elsif ($ext =~ /jpe?g/i) {
        $result = "wxBITMAP_TYPE_JPEG";
    } else {
        print "ERROR: bad format $ext\n";
    }

    return $result;
}

my $input=$ARGV[0];
my $output=$ARGV[1] . ".h" ;

my($filename, $directories, $suffix) = fileparse($input);
my ($name, $ext) = split(/\./,$filename);

my $wx_img_class = "res_$name";
my $wx_img_extension = get_wx_ext($ext);
my $filesize = -s $input;

### Collect binary data
my $lenght = 1;
my $binary = "\t\t";
my $data;
my $byte;

open(IN,"$input");
binmode IN;
while (($byte = read IN, $data, 1) != 0) {
    my $hex = ascii_to_hex($data);
    $binary .= "0x$hex";
    if ($lenght % 17 == 0 && $lenght > 1) {
        # End of line
        $binary .= ",\n\t\t";
    } elsif ($filesize == $lenght) {
        # End of file
        $binary .= "\n";
    } else {
        $binary .= ",";
    }
    $lenght++;
}
close(IN);

open(OUT,">$output");
### Print the header
print OUT "#pragma once\n\n";
print OUT "#include \"Pcsx2Types.h\"\n";
print OUT "#include <wx/gdicmn.h>\n\n";
print OUT "class $wx_img_class\n{\n";
print OUT "public:\n";
print OUT "\tstatic const uint Length = $filesize;\n";
print OUT "\tstatic const u8 Data[Length];\n";
print OUT "\tstatic wxBitmapType GetFormat() { return $wx_img_extension; }\n};\n\n";
print OUT "const u8 ${wx_img_class}::Data[Length] =\n{\n";

### Print the array
print OUT $binary;
print OUT "};\n";

close(OUT);

