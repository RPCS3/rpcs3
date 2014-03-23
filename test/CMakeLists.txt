add_subdirectory(gtest-1.7.0)
include_directories(gtest-1.7.0/include)

file(GLOB test_headers [a-z_]*.h)
file(GLOB test_sources [a-z_]*.cpp)
file(GLOB test_core_sources core/[a-z]*.cpp)

list(APPEND test_sources ${test_core_sources})

include_directories(${YAML_CPP_SOURCE_DIR}/test)

add_executable(run-tests
	${test_sources}
	${test_headers}
)
target_link_libraries(run-tests yaml-cpp gtest)

#add_test(yaml-reader-test run-tests)
