option (UPNPC_BUILD_STATIC "Build static library" TRUE)
option (UPNPC_BUILD_SHARED "Build shared library" FALSE)
option (UPNPC_BUILD_TESTS "Build test executables" FALSE)
option (UPNPC_BUILD_SAMPLE "Build sample executables" FALSE)
option (NO_GETADDRINFO "Define NO_GETADDRINFO" FALSE)
option (UPNPC_NO_INSTALL "Disable installation" TRUE)

add_subdirectory(miniupnp/miniupnpc EXCLUDE_FROM_ALL)
