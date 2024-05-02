option(RTMIDI_API_JACK "Compile with JACK support." OFF)
option(RTMIDI_BUILD_TESTING "Build test programs" OFF)
set(RTMIDI_TARGETNAME_UNINSTALL "uninstall-rpcs3-rtmidi")
add_subdirectory(rtmidi EXCLUDE_FROM_ALL)
