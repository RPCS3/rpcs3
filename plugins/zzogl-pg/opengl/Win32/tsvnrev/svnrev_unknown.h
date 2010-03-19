// svnrev_genric.h --> svnrev.h
//
// This file acts as a placebo for people who do not have TortoiseSVN installed.
// It provides "empty" revision information to the Pcsx2 Playground projects in
// the absence of real revisions derived from the repository being built.
// 
// This file does not affect application/dll builds in any significant manner,
// other than the lack of automatic revision tags inserted into the app (which
// is very convenient but hardly necessary).
//
// See svn_template.h for more information on how the process of revision
// templating works.
//
// If you would like to enable automatic revisin tagging, TortoiseSVN can be
// downloaded from http://tortoisesvn.tigris.org

#define SVN_REV_UNKNOWN

// The following defines are included so that code will still compile even if it
// doesn't check for the SVN_REV_UNKNOWN define.

#define SVN_REV 0
#define SVN_MODS ""