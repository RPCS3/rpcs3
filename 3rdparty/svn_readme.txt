
Folder: /3rdparty

This folder contains libraries written and (un?)maintained by people or parties
outside the Pcsx2 DevTeam.  Libraries or code snippets written by the PCSX2 team
are housed in /common instead.

Cross-Compilation:

Most libraries includes in /3rdparty come as either default installations in Linux
distributions, or are easily obtainable.  Using the PCSX2 versions included here is
optional in such cases, however using the distro-provided packages is not recommended.
(using PCSX2 included versions may help resolve versioning issues).

Note that wxWidgets includes *only* MSW projects and files.  Linux and Mac platforms
are assumed to have wx already available in your distributions.

Modifications:

Most of these libs in /3rdparty have been tailored slightly from the original
forms downloaded from the net.  So if you want to upgrade to a new version you
will need to do a proper diff merge.  Likewise, if you modify anything in a
3rdparty library you should be sure to tag the modification with your sig or
a "pcsx2" or something.  Thirdly, you can use Svn Log to check for modifications,
however folder renames or svn homesite changes may not have all relevant history
necessary to make an accurate merge.