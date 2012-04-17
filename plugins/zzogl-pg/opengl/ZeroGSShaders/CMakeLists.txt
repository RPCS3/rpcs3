# Check that people use the good file
if(NOT TOP_CMAKE_WAS_SOURCED)
    message(FATAL_ERROR "
    You did not 'cmake' the good CMakeLists.txt file. Use the one in the top dir.
    It is advice to delete all wrongly generated cmake stuff => CMakeFiles & CMakeCache.txt")
endif(NOT TOP_CMAKE_WAS_SOURCED)


# plugin name
set(Output zzogl-shader)

set(CommonFlags
    -pthread
    -DZEROGS_SSE2
    -fno-strict-aliasing
    -Wstrict-aliasing # Allow to track strict aliasing issue.
    -Wunused-variable
    -DNVIDIA_CG_API
    )

set(OptimizationFlags
    -O2
    -DNDEBUG
    )

# Debug - Build
if(CMAKE_BUILD_TYPE STREQUAL Debug)
    # add defines
    add_definitions(${CommonFlags} -g -Wall -D_DEBUG)
endif(CMAKE_BUILD_TYPE STREQUAL Debug)

# Devel - Build
if(CMAKE_BUILD_TYPE STREQUAL Devel)
    # add defines
    add_definitions(${CommonFlags} ${OptimizationFlags} -g -W -DZEROGS_DEVBUILD)
endif(CMAKE_BUILD_TYPE STREQUAL Devel)

# Release - Build
if(CMAKE_BUILD_TYPE STREQUAL Release)
    # add defines
    add_definitions(${CommonFlags} ${OptimizationFlags} -W)
endif(CMAKE_BUILD_TYPE STREQUAL Release)

include_directories(.)

# add library
add_executable(${Output} zerogsshaders.cpp zpipe.cpp)

# link target with zlib
target_link_libraries(${Output} ${ZLIB_LIBRARIES})

# link target with Cg
target_link_libraries(${Output} ${CG_LIBRARIES})

# link target with opengl
target_link_libraries(${Output} ${OPENGL_LIBRARIES})

# User flags options
if(NOT USER_CMAKE_LD_FLAGS STREQUAL "")
    target_link_libraries(${Output} "${USER_CMAKE_LD_FLAGS}")
endif(NOT USER_CMAKE_LD_FLAGS STREQUAL "")

# Now build the shader
add_custom_command(TARGET ${Output} POST_BUILD
    COMMAND ${Output} ps2hw.fx ${CMAKE_CURRENT_BINARY_DIR}/ps2hw_cmake.dat
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/plugins/zzogl-pg/opengl
    )

if(PACKAGE_MODE)
    install(TARGETS ${Output} DESTINATION bin)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/ps2hw_cmake.dat DESTINATION ${PLUGIN_DIR})
else(PACKAGE_MODE)
    install(TARGETS ${Output} DESTINATION ${CMAKE_SOURCE_DIR}/bin)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/ps2hw_cmake.dat DESTINATION ${CMAKE_SOURCE_DIR}/bin/plugins)
endif(PACKAGE_MODE)
