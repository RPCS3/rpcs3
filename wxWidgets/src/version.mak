##############################################################################
# Name:        src/version.mak
# Purpose:     file defining wxWindows version used by all other makefiles
# Author:      Vadim Zeitlin
# Modified by:
# Created:     25.02.03
# RCS-ID:      $Id: version.mak 23584 2003-09-14 19:39:34Z JS $
# Copyright:   (c) 2003 Vadim Zeitlin
# Licence:     wxWindows license
##############################################################################

wxMAJOR_VERSION=2
wxMINOR_VERSION=5
wxRELEASE_NUMBER=1

# release number if used in the DLL file names only for the unstable branch as
# for the stable branches the micro releases are supposed to be backwards
# compatible and so should have the same name or otherwise it would be
# impossible to use them without recompiling the applications (which is the
# whole goal of keeping them backwards compatible in the first place)
#
# as 2.5 is an unstable branch, wxRELEASE_NUMBER_IFUNSTABLE should be set
# (but when we go to 2.6, it should become empty)
wxRELEASE_NUMBER_IFUNSTABLE=$(wxRELEASE_NUMBER)
