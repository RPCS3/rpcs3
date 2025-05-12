option(SDL_SHARED "Build a shared version of the library" OFF)
option(SDL_STATIC "Build a static version of the library" ON)
option(SDL_TEST_LIBRARY "Build the SDL3_test library" OFF)
add_subdirectory(SDL EXCLUDE_FROM_ALL)
