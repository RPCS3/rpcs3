SET(RPCS3_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(PROJ_NAME rpcs3)

include(cotire)

#Find wxWidgets
find_package(wxWidgets COMPONENTS core base net aui gl xml REQUIRED)
include(${wxWidgets_USE_FILE})

#Find GLEW
find_package(GLEW REQUIRED)

#Find GLM
find_package(GLM REQUIRED)

#Find GLEXT
find_package(GLEXT REQUIRED)

#Find OpenAL
find_package(OpenAL REQUIRED)

#Find DirectX
find_package(DirectX REQUIRED)

#Find FFMPEG
find_package(FFMPEG REQUIRED)

#Find Vulkan
find_package(Vulkan REQUIRED)

#Find LibPNG
find_package(PNG REQUIRED)

#Embed asmjit
set(ASMJIT_EMBED TRUE)
add_Definitions(-DASMJIT_STATIC)
set(ASMJIT_DIR ${CMAKE_SOURCE_DIR}/asmjit)
include(${ASMJIT_DIR}/CMakeLists.txt)

file(
GLOB_RECURSE
RPCS3_SRC
"${RPCS3_SRC_DIR}/rpcs3.cpp"
"${RPCS3_SRC_DIR}/config.cpp"
"${RPCS3_SRC_DIR}/stb_image.cpp"
"${RPCS3_SRC_DIR}/../Utilities/GNU.cpp"
"${RPCS3_SRC_DIR}/Emu/*"
"${RPCS3_SRC_DIR}/Gui/*"
"${RPCS3_SRC_DIR}/Loader/*"
"${RPCS3_SRC_DIR}/Crypto/*"
"${RPCS3_SRC_DIR}/../Utilities/*"
"${RPCS3_SRC_DIR}/../rsx_program_decompiler/rsx_decompiler/*"
"${RPCS3_SRC_DIR}/../rsx_program_decompiler/shader_code/*"
)

add_custom_target(GitVersion ALL
	DEPENDS something_that_never_exists)
add_custom_command(OUTPUT something_that_never_exists
	COMMAND ${CMAKE_COMMAND} -DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
		-P ${CMAKE_CURRENT_SOURCE_DIR}/git-version.cmake)


add_executable(${PROJ_NAME} ${RPCS3_SRC})

set_target_properties(${PROJ_NAME} PROPERTIES COTIRE_CXX_PREFIX_HEADER_INIT "${RPCS3_SRC_DIR}/stdafx.h")
cotire(${PROJ_NAME})

target_include_directories(${PROJ_NAME} PUBLIC 	${wxWidgets_INCLUDE_DIRS}
												${GLEW_INCLUDE_DIRS}
												${GLM_INCLUDE_DIRS}
												${GLEXT_INCLUDE_DIRS}
												${PNG_INCLUDE_DIRS}
												${OPENAL_INCLUDE_DIR}
												${FFMPEG_INCLUDE_DIR}
												${VULKAN_INCLUDE_DIR}
												${ASMJIT_INCLUDE_DIR}
												${RPCS3_SRC_DIR}
												${RPCS3_SRC_DIR}/..
												${RPCS3_SRC_DIR}/../asmjit/src/asmjit
												${RPCS3_SRC_DIR}/../GSL/include
												${RPCS3_SRC_DIR}/../rsx_program_decompiler/shader_code)

target_link_libraries(${PROJ_NAME} 	${wxWidgets_LIBRARIES}
									${PNG_LIBRARIES}
									${GLEW_LIBRARIES}
									${OPENAL_LIBRARY}
									${VULKAN_LIBRARY}
									${FFMPEG_LIBRARIES}
									${ASMJIT_DEPS})

#Use C++ 14
set_property(TARGET ${PROJ_NAME} PROPERTY CXX_STANDARD 14)
set_property(TARGET ${PROJ_NAME} PROPERTY CXX_STANDARD_REQUIRED  TRUE)

