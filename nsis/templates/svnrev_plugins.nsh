!if ${USE_PACKAGE_REV} == 0
  !define SVNREV_PLUGINS     $WCREV$
  !define SVNMOD_PLUGINS     $WCMODS?1:0$
!else
  !define SVNREV_PLUGINS     ${SVNREV_PACKAGE}
  !define SVNMOD_PLUGINS     ${SVMOD_PACKAGE}
!endif
