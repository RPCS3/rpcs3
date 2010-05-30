!if ${USE_PACKAGE_REV} == 0
  !define SVNREV_CDVDISO      $WCREV$
!else
  !define SVNREV_CDVDISO      ${SVNREV_PACKAGE}
!endif
