!if ${USE_PACKAGE_REV} == 0
  !define SVNREV_ZZOGL      $WCREV$
!else
  !define SVNREV_ZZOGL      ${SVNREV_PACKAGE}
!endif