!if ${USE_PACKAGE_REV} == 0
  !define SVNREV_PCSX2     $WCREV$
  !define SVNMOD_PCSX2     $WCMODS?1:0$
!else
  !define SVNREV_PCSX2     ${SVNREV_PACKAGE}
  !define SVNMOD_PCSX2     ${SVMOD_PACKAGE}
!endif

