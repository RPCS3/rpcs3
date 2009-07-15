prefix=@CMAKE_INSTALL_PREFIX@
exec_prefix=@CMAKE_INSTALL_PREFIX@
libdir=@LIB_INSTALL_DIR@
includedir=@INCLUDE_INSTALL_DIR@

Name: Yaml-cpp
Description: A YAML parser and emitter for C++
Version: @YAML_CPP_VERSION@
Requires:
Libs: -L${libdir} -lyaml-cpp
Cflags: -I${includedir}
