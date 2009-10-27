file(GLOB yaml-reader_headers [a-z]*.h)
file(GLOB yaml-reader_sources [a-z]*.cpp)

add_executable(yaml-reader
	${yaml-reader_sources}
	${yaml-reader_headers}
)
target_link_libraries(yaml-reader yaml-cpp)

add_test(yaml-reader-test yaml-reader)
