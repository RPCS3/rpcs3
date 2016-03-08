SET(RPCS3_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(PROJ_NAME rpcs3)

include(cotire)

#Find wxWidgets
set(wxWidgets_CONFIGURATION mswu)
find_package(wxWidgets REQUIRED core base net aui gl xml)
include(${wxWidgets_USE_FILE})

#Find GLEW
find_package(GLEW REQUIRED)

#Find GLM
find_package(GLM REQUIRED)

#Find GLEXT
find_package(GLEXT REQUIRED)

#Find OpenAL
find_package(OpenAL REQUIRED)

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

set(CMAKE_FLAGS)
set(D3D12_SRC)
set(D3D12_STDAFX)
set(ADDITIONAL_LIBS)
if(WIN32)
	#Find DirectX
	find_package(DirectX REQUIRED)

	option(BUILD_D3D12_RENDERER "Add the Direct3D12 Renderbackend" ON)

	if(BUILD_D3D12_RENDERER)
		file(
		GLOB_RECURSE
		D3D12_SRC
		"${RPCS3_SRC_DIR}/Emu/RSX/D3D12/D3D12Buffer.cpp"
		"${RPCS3_SRC_DIR}/Emu/RSX/D3D12/D3D12CommonDecompiler.cpp"
		"${RPCS3_SRC_DIR}/Emu/RSX/D3D12/D3D12Formats.cpp"
		"${RPCS3_SRC_DIR}/Emu/RSX/D3D12/D3D12FragmentProgramDecompiler.cpp"
		"${RPCS3_SRC_DIR}/Emu/RSX/D3D12/D3D12Render.cpp"
		"${RPCS3_SRC_DIR}/Emu/RSX/D3D12/D3D12MemoryHelpers.cpp"
		"${RPCS3_SRC_DIR}/Emu/RSX/D3D12/D3D12Overlay.cpp"
		"${RPCS3_SRC_DIR}/Emu/RSX/D3D12/D3D12PipelineState.cpp"
		"${RPCS3_SRC_DIR}/Emu/RSX/D3D12/D3D12RenderTargetSets.cpp"
		"${RPCS3_SRC_DIR}/Emu/RSX/D3D12/D3D12Texture.cpp"
		"${RPCS3_SRC_DIR}/Emu/RSX/D3D12/D3D12Utils.cpp"
		"${RPCS3_SRC_DIR}/Emu/RSX/D3D12/D3D12VertexProgramDecompiler.cpp"
		)
		set(D3D12_STDAFX "${RPCS3_SRC_DIR}/stdafx_d3d12.h")
	endif()
	set(CMAKE_FLAGS WIN32)
	set(ADDITIONAL_LIBS "iphlpapi" "Ws2_32")
endif()

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
add_executable(${PROJ_NAME} ${CMAKE_FLAGS} ${RPCS3_SRC} ${D3D12_SRC} ${ASMJIT_SRC})

add_custom_command(TARGET ${PROJ_NAME} PRE_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                       ${CMAKE_SOURCE_DIR}/bin $<TARGET_FILE_DIR:${PROJ_NAME}>)

set_target_properties(${PROJ_NAME} PROPERTIES COTIRE_CXX_PREFIX_HEADER_INIT "${RPCS3_SRC_DIR}/stdafx.h" "${RPCS3_SRC_DIR}/stdafx_gui.h" "${D3D12_STDAFX}")
cotire(${PROJ_NAME})

target_include_directories(${PROJ_NAME} PUBLIC 	${wxWidgets_INCLUDE_DIRS}
												${GLEW_INCLUDE_DIRS}
												${GLM_INCLUDE_DIRS}
												${GLEXT_INCLUDE_DIRS}
												${PNG_INCLUDE_DIRS}
												${OPENAL_INCLUDE_DIR}
												${FFMPEG_INCLUDE_DIRS}
												${VULKAN_INCLUDE_DIR}
												${ASMJIT_INCLUDE_DIR}
												${DIRECTX_INCLUDE_DIRS}
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
									${ASMJIT_DEPS}
									${DIRECTX_XINPUT_LIBRARY}
									${DIRECTX_XAUDIO_LIBRARY}
									${DIRECTX_DWRITE_LIBRARY}
									${DIRECTX_DXGI_LIBRARY}
									${DIRECTX_D2D1_LIBRARY}
									${ADDITIONAL_LIBS})


#Use C++ 14
set_property(TARGET ${PROJ_NAME} PROPERTY CXX_STANDARD 14)
set_property(TARGET ${PROJ_NAME} PROPERTY CXX_STANDARD_REQUIRED  TRUE)

