!if ${USE_PACKAGE_REV} == 0
  !define SVNREV_GSDX      $WCREV$
!else
  !define SVNREV_GSDX      ${SVNREV_PACKAGE}
!endif
