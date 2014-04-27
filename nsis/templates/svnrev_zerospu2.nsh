!if ${USE_PACKAGE_REV} == 0
  !define SVNREV_ZEROSPU2      $WCREV$
!else
  !define SVNREV_ZEROSPU2      ${SVNREV_PACKAGE}
!endif