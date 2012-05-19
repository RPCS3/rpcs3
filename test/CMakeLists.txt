file(GLOB test_headers [a-z]*.h)
file(GLOB test_sources [a-z]*.cpp)
file(GLOB test_new_api_sources new-api/[a-z]*.cpp)

list(APPEND test_sources ${test_new_api_sources})

include_directories(${YAML_CPP_SOURCE_DIR}/test)

add_executable(run-tests
	${test_sources}
	${test_headers}
)
target_link_libraries(run-tests yaml-cpp)

add_test(yaml-reader-test run-tests)
