# To avoid python numpy dependency in debug build, force release build of this library
set(ORIG_BUILD_TYPE ${CMAKE_BUILD_TYPE})
set(CMAKE_BUILD_TYPE Release)
add_subdirectory(fusion EXCLUDE_FROM_ALL)
set(CMAKE_BUILD_TYPE ${ORIG_BUILD_TYPE})
