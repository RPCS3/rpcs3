file(GLOB SRC_FILES "*.cpp")

add_library(rpcs3_loader ${SRC_FILES})
target_include_directories(rpcs3_loader PUBLIC .)
target_link_libraries(rpcs3_loader PUBLIC rpcs3_utilities)
