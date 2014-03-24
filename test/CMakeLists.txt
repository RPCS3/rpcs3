set(gtest_force_shared_crt ${MSVC_SHARED_RT} CACHE BOOL
  "Use shared (DLL) run-time lib even when Google Test built as a static lib.")
add_subdirectory(gmock-1.7.0)
include_directories(gmock-1.7.0/gtest/include)
include_directories(gmock-1.7.0/include)

if(WIN32 AND BUILD_SHARED_LIBS)
  add_definitions("-DGTEST_LINKED_AS_SHARED_LIBRARY")
endif()

file(GLOB test_headers [a-z_]*.h)
file(GLOB test_sources [a-z_]*.cpp integration/[a-z_]*.cpp)
file(GLOB test_core_sources core/[a-z]*.cpp)

list(APPEND test_sources ${test_core_sources})

include_directories(${YAML_CPP_SOURCE_DIR}/test)

add_executable(run-tests
	${test_sources}
	${test_headers}
)
target_link_libraries(run-tests yaml-cpp gtest gmock)

#add_test(yaml-reader-test run-tests)
