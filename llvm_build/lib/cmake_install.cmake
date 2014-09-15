# Install script for directory: D:/Projects/rpcs3/llvm/lib

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "C:/Program Files/LLVM")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("D:/Projects/rpcs3/llvm_build/lib/IR/cmake_install.cmake")
  include("D:/Projects/rpcs3/llvm_build/lib/IRReader/cmake_install.cmake")
  include("D:/Projects/rpcs3/llvm_build/lib/CodeGen/cmake_install.cmake")
  include("D:/Projects/rpcs3/llvm_build/lib/Bitcode/cmake_install.cmake")
  include("D:/Projects/rpcs3/llvm_build/lib/Transforms/cmake_install.cmake")
  include("D:/Projects/rpcs3/llvm_build/lib/Linker/cmake_install.cmake")
  include("D:/Projects/rpcs3/llvm_build/lib/Analysis/cmake_install.cmake")
  include("D:/Projects/rpcs3/llvm_build/lib/LTO/cmake_install.cmake")
  include("D:/Projects/rpcs3/llvm_build/lib/MC/cmake_install.cmake")
  include("D:/Projects/rpcs3/llvm_build/lib/Object/cmake_install.cmake")
  include("D:/Projects/rpcs3/llvm_build/lib/Option/cmake_install.cmake")
  include("D:/Projects/rpcs3/llvm_build/lib/DebugInfo/cmake_install.cmake")
  include("D:/Projects/rpcs3/llvm_build/lib/ExecutionEngine/cmake_install.cmake")
  include("D:/Projects/rpcs3/llvm_build/lib/Target/cmake_install.cmake")
  include("D:/Projects/rpcs3/llvm_build/lib/AsmParser/cmake_install.cmake")
  include("D:/Projects/rpcs3/llvm_build/lib/LineEditor/cmake_install.cmake")
  include("D:/Projects/rpcs3/llvm_build/lib/ProfileData/cmake_install.cmake")

endif()

