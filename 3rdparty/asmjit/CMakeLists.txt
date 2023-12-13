set(ASMJIT_EMBED TRUE)
set(ASMJIT_STATIC TRUE)
set(ASMJIT_BUILD_X86 TRUE)
set(ASMJIT_BUILD_ARM FALSE)
set(ASMJIT_BUILD_TEST FALSE)
set(ASMJIT_NO_DEPRECATED TRUE)
set(ASMJIT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/asmjit" CACHE PATH "Location of 'asmjit'")

include("${ASMJIT_DIR}/CMakeLists.txt")

add_library(asmjit ${ASMJIT_SRC})
target_include_directories(asmjit PUBLIC ${ASMJIT_DIR}/src)
target_link_libraries(asmjit PRIVATE ${ASMJIT_DEPS})

# ASMJIT should have a option for disabling installing and this wouldnt
# be required to avoid installing ASMJIT...

set_property(TARGET asmjit PROPERTY FOLDER "3rdparty/")
add_library(3rdparty::asmjit ALIAS asmjit)
