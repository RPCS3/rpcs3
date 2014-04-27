!if ${USE_PACKAGE_REV} == 0
  !define SVNREV_SPU2X      $WCREV$
!else
  !define SVNREV_SPU2X      ${SVNREV_PACKAGE}
!endif