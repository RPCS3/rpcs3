# Check and configure compiler options for RPCS3

if(CMAKE_COMPILER_IS_GNUCXX)
	# GCC 8.1 or latter is required
	if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 8)
		message(FATAL_ERROR "RPCS3 requires at least gcc-8.")
	endif()

	# Set compiler options here

	# Warnings
	add_compile_options(-Wall)
	add_compile_options(-Wno-attributes -Wno-enum-compare -Wno-invalid-offsetof)
	add_compile_options(-Wno-unknown-pragmas -Wno-unused-variable -Wno-reorder -Wno-comment)

elseif(${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
	# Clang 5.0 or latter is required
	if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 5.0)
		message(FATAL_ERROR "RPCS3 requires at least clang-5.0.")
	endif()

	# Set compiler options here

	add_compile_options(-ftemplate-depth=1024)
	add_compile_options(-Wunused-value -Wunused-comparison)
	if(APPLE)
		add_compile_options(-stdlib=libc++)
	endif()
endif()

if(NOT MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-invalid-pch -fexceptions")
	if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-command-line-argument")
	endif()

	# This hides our LLVM from mesa's LLVM, otherwise we get some unresolvable conflicts.
	if(NOT APPLE)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,--exclude-libs,ALL")
	endif()

	if(WIN32)
		set(CMAKE_RC_COMPILER_INIT windres)
		enable_language(RC)
		set(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> <FLAGS> -O coff <DEFINES> -i <SOURCE> -o <OBJECT>")

		# Workaround for mingw64 (MSYS2)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,--allow-multiple-definition")

		# Increase stack limit to 8 MB
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,--stack -Wl,8388608")

		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__STDC_FORMAT_MACROS=1")
	endif()

	add_compile_options(-msse -msse2 -mcx16)

	if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
		# This fixes 'some' of the st11range issues. See issue #2516
		if(APPLE)
			set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-image_base,0x10000 -Wl,-pagezero_size,0x10000")
		else()
			set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -image-base=0x10000")
		endif()
	endif()

	# Some distros have the compilers set to use PIE by default, but RPCS3 doesn't work with PIE, so we need to disable it.
	if(APPLE)
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-no_pie")
	else()
		CHECK_CXX_COMPILER_FLAG("-no-pie" HAS_NO_PIE)
		CHECK_CXX_COMPILER_FLAG("-nopie" HAS_NOPIE)

		if(HAS_NO_PIE)
			set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -no-pie")
		elseif(HAS_NOPIE)
			set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -nopie")
		endif()
	endif()
else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zc:throwingNew /D _CRT_SECURE_NO_DEPRECATE=1 /D _CRT_NON_CONFORMING_SWPRINTFS=1 /D _SCL_SECURE_NO_WARNINGS=1")

	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D _ENABLE_EXTENDED_ALIGNED_STORAGE=1")

	# Increase stack limit to 8 MB
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /STACK:8388608,1048576")

	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB:libc.lib /NODEFAULTLIB:libcmt.lib /NODEFAULTLIB:libcd.lib /NODEFAULTLIB:libcmtd.lib /NODEFAULTLIB:msvcrtd.lib")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS /DYNAMICBASE:NO /BASE:0x10000 /FIXED")
endif()

if(USE_NATIVE_INSTRUCTIONS)
	CHECK_CXX_COMPILER_FLAG("-march=native" COMPILER_SUPPORTS_MARCH_NATIVE)
	if(COMPILER_SUPPORTS_MARCH_NATIVE)
		add_compile_options(-march=native)
	endif()
endif()
