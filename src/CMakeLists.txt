set(FILES content.cpp iterator.cpp node.cpp parserstate.cpp
          scalar.cpp scanscalar.cpp sequence.cpp stream.cpp
          exp.cpp map.cpp parser.cpp regex.cpp scanner.cpp
          scantoken.cpp simplekey.cpp)

include_directories(${YAML_CPP_SOURCE_DIR}/include)
add_library(yaml-cpp ${FILES})
