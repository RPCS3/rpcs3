cmake_minimum_required(VERSION 2.6)

project (YAML_CPP)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

set(PUB_HDR
	include/conversion.h
	include/crt.h
	include/emitter.h
	include/emittermanip.h
	include/exceptions.h
	include/iterator.h
	include/node.h
	include/noncopyable.h
	include/ostream.h
	include/parser.h
	include/parserstate.h
	include/stlemitter.h
	include/yaml.h
)

set(PVT_HDR
	src/alias.h
	src/content.h
	src/emitterstate.h
	src/emitterutils.h
	src/exp.h
	src/indentation.h
	src/iterpriv.h
	src/ltnode.h
	src/map.h
	src/regex.h
	src/scalar.h
	src/scanner.h
	src/scanscalar.h
	src/sequence.h
	src/setting.h
	src/stream.h
	src/token.h
)

set(SRC
	src/alias.cpp
	src/content.cpp
	src/conversion.cpp
	src/emitter.cpp
	src/emitterstate.cpp
	src/emitterutils.cpp
	src/exp.cpp
	src/iterator.cpp
	src/map.cpp
	src/node.cpp
	src/ostream.cpp
	src/parser.cpp
	src/parserstate.cpp
	src/regex.cpp
	src/scalar.cpp
	src/scanner.cpp
	src/scanscalar.cpp
	src/scantoken.cpp
	src/sequence.cpp
	src/simplekey.cpp
	src/stream.cpp
)

include_directories(${YAML_CPP_SOURCE_DIR}/include)
add_library(yaml-cpp
	${PUB_HDR}
	${PVT_HDR}
	${SRC}
)

add_subdirectory (yaml-reader)
