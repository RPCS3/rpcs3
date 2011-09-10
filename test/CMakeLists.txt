file(GLOB test_headers [a-z]*.h)
file(GLOB test_sources [a-z]*.cpp)
file(GLOB test_old_api_sources old-api/[a-z]*.cpp)

if(YAML_CPP_BUILD_OLD_API)
	list(APPEND test_sources ${test_old_api_sources})
endif()

add_executable(run-tests
	${test_sources}
	${test_headers}
)
target_link_libraries(run-tests yaml-cpp)

add_test(yaml-reader-test run-tests)
