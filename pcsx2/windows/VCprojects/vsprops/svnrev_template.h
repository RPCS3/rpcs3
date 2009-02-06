// svnrev_template.h --> svnrev.h
//
// This file acts as a template for the automatic SVN revision/version tag.
// It is used by the utility SubWCrev.exe to create an "svnrev.h" file for
// whichever project is being compiled (as indicated by command line options
// passed to SubWCRev.exe during the project's pre-build step).
//
// The SubWCRev.exe utility is part of TortoiseSVN and requires several DLLs
// installed by TortoiseSVN, so it will only be available if you have TortoiseSVN
// installed on your system.  If you do not have it installed, a generic template
// is used instead (see svnrev_generic.h).  Having TortoiseSVN is handy but not
// necessary.  If you do not have it installed, everything will still compile
// fine except without the SVN revision tagged to the application/dll version.
//
// TortoiseSVN can be downloaded from http://tortoisesvn.tigris.org

#define SVN_REV $WCREV$
#define SVN_MODS $WCMODS?1:0$