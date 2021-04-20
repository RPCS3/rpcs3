# Check and configure compiler options for RPCS3

if(MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zc:throwingNew- /constexpr:steps16777216 /D _CRT_SECURE_NO_DEPRECATE=1 /D _CRT_NON_CONFORMING_SWPRINTFS=1 /D _SCL_SECURE_NO_WARNINGS=1")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D NOMINMAX /D _ENABLE_EXTENDED_ALIGNED_STORAGE=1 /D _HAS_EXCEPTIONS=0 /MT")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB:libc.lib /NODEFAULTLIB:libcmt.lib /NODEFAULTLIB:libcd.lib /NODEFAULTLIB:libcmtd.lib /NODEFAULTLIB:msvcrtd.lib")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS /DYNAMICBASE:NO /BASE:0x10000 /FIXED")

	#TODO: Some of these could be cleaned up
	add_compile_options(/wd4805) # Comparing boolean and int
	add_compile_options(/wd4804) # Using integer operators with booleans
	add_compile_options(/wd4200) # Zero-sized array in struct/union
	add_link_options(/ignore:4281) # Undesirable base address 0x10000

	# MSVC 2017 uses iterator as base class internally, causing a lot of warning spam
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D _SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING=1")

	# Increase stack limit to 8 MB
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /STACK:8388608,1048576")
else()
	# Some distros have the compilers set to use PIE by default, but RPCS3 doesn't work with PIE, so we need to disable it.
	CHECK_CXX_COMPILER_FLAG("-no-pie" HAS_NO_PIE)
	CHECK_CXX_COMPILER_FLAG("-march=native" COMPILER_SUPPORTS_MARCH_NATIVE)

	add_compile_options(-Wall)
	add_compile_options(-fno-exceptions)
	add_compile_options(-fstack-protector)
	add_compile_options(-msse -msse2 -mcx16)

	if ((${CMAKE_CXX_COMPILER_ID} MATCHES "GNU") AND (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 10.1))
		add_compile_options(-fconcepts)
	endif()

	add_compile_options(-Werror=old-style-cast)
	add_compile_options(-Werror=sign-compare)
	add_compile_options(-Werror=reorder)
	add_compile_options(-Werror=return-type)
	add_compile_options(-Werror=overloaded-virtual)
	add_compile_options(-Werror=missing-noreturn)
	add_compile_options(-Wunused-parameter)
	add_compile_options(-Wignored-qualifiers)
	add_compile_options(-Wredundant-move)
	add_compile_options(-Wcast-qual)
	add_compile_options(-Wdeprecated-copy)
	add_compile_options(-Wtautological-compare)
	#add_compile_options(-Wshadow)
	#add_compile_options(-Wconversion)
	#add_compile_options(-Wpadded)
	add_compile_options(-Wempty-body)
	add_compile_options(-Wredundant-decls)
	#add_compile_options(-Weffc++)

	add_compile_options(-Wstrict-aliasing=1)
	#add_compile_options(-Wnull-dereference)

	if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
		add_compile_options(-Werror=inconsistent-missing-override)
	elseif(${CMAKE_CXX_COMPILER_ID} MATCHES "GNU")
		add_compile_options(-Werror=suggest-override)
		add_compile_options(-Wclobbered)
		add_compile_options(-Wcast-function-type)
		add_compile_options(-Wduplicated-branches)
		add_compile_options(-Wduplicated-cond)
	endif()

	#TODO Clean the code so these are removed
	if (NOT (${CMAKE_CXX_COMPILER_ID} MATCHES "GNU") OR (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 10.1))
		add_compile_options(-Wno-attributes)
	endif()

	if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
		add_compile_options(-fconstexpr-steps=16777216)
		add_compile_options(-Wno-unused-lambda-capture)
		add_compile_options(-Wno-unused-private-field)
		add_compile_options(-Wno-delete-non-virtual-dtor)
		add_compile_options(-Wno-unused-command-line-argument)
	elseif(${CMAKE_CXX_COMPILER_ID} MATCHES "GNU")
		add_compile_options(-Wno-class-memaccess)
	endif()

	if(USE_NATIVE_INSTRUCTIONS AND COMPILER_SUPPORTS_MARCH_NATIVE)
		add_compile_options(-march=native)
	endif()

	if(NOT APPLE AND NOT WIN32)
		# This hides our LLVM from mesa's LLVM, otherwise we get some unresolvable conflicts.
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,--exclude-libs,ALL")

		if(HAS_NO_PIE)
			set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -no-pie")
		endif()
	elseif(APPLE)
		add_compile_options(-stdlib=libc++)
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-image_base,0x10000 -Wl,-pagezero_size,0x10000")
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-no_pie")
	elseif(WIN32)
		set(CMAKE_RC_COMPILER_INIT windres)
		enable_language(RC)
		set(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> <FLAGS> -O coff <DEFINES> -i <SOURCE> -o <OBJECT>")

		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__STDC_FORMAT_MACROS=1")

		# Workaround for mingw64 (MSYS2)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,--allow-multiple-definition")

		# Increase stack limit to 8 MB
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,--stack -Wl,8388608")
	endif()
endif()
