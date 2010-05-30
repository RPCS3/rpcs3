!if ${USE_PACKAGE_REV} == 0
  !define SVNREV_LILYPAD      $WCREV$
!else
  !define SVNREV_LILYPAD      ${SVNREV_PACKAGE}
!endif