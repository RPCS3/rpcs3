project(moltenvk NONE)
include(ExternalProject)

ExternalProject_Add(moltenvk
	GIT_REPOSITORY https://github.com/KhronosGroup/MoltenVK.git
	GIT_TAG 49b97f2
	BUILD_IN_SOURCE 1
	SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/MoltenVK
	CONFIGURE_COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/MoltenVK/fetchDependencies" --macos
	BUILD_COMMAND xcodebuild build -quiet -project "${CMAKE_CURRENT_SOURCE_DIR}/MoltenVK/MoltenVKPackaging.xcodeproj" -scheme "MoltenVK Package \(macOS only\)" -configuration "Release" -arch "${CMAKE_HOST_SYSTEM_PROCESSOR}"
	COMMAND ln -f "${CMAKE_CURRENT_SOURCE_DIR}/MoltenVK/MoltenVK/dylib/macOS/libMoltenVK.dylib" "${CMAKE_CURRENT_SOURCE_DIR}/MoltenVK/Build/Products/Release/dynamic/libMoltenVK.dylib"
	INSTALL_COMMAND ""
	BUILD_BYPRODUCTS "${CMAKE_CURRENT_SOURCE_DIR}/MoltenVK/Build/Products/Release/dynamic/libMoltenVK.dylib"
)
