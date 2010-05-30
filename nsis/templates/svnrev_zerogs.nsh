!if ${USE_PACKAGE_REV} == 0
  !define SVNREV_ZEROGS      $WCREV$
!else
  !define SVNREV_ZEROGS      ${SVNREV_PACKAGE}
!endif