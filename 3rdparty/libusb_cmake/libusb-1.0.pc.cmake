prefix=@CMAKE_INSTALL_PREFIX@
libdir=@CMAKE_INSTALL_PREFIX@/lib@
includedir=@CMAKE_INSTALL_PREFIX@/include@

Name: libusb-1.0
Description: C API for USB device access from Linux, Mac OS X and Windows userspace
Version: @VERSION@
Libs: -L${libdir} -lusb-1.0
Libs.private: @LIBUSB_LIB_DEPENDS@
Cflags: -I${includedir}/libusb-1.0

