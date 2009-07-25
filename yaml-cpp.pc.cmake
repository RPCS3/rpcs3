prefix=@CMAKE_INSTALL_PREFIX@
exec_prefix=@CMAKE_INSTALL_PREFIX@
libdir=${prefix}/@LIB_INSTALL_DIR@
includedir=${prefix}/@INCLUDE_INSTALL_DIR@

Name: Yaml-cpp
Description: A YAML parser and emitter for C++
Version: @YAML_CPP_VERSION@
Requires:
Libs: -L${libdir} -lyaml-cpp
Cflags: -I${includedir}
