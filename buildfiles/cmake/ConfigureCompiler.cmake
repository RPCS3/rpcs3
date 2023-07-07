# Check and configure compiler options for RPCS3

if(MSVC)
	add_compile_options(/Zc:throwingNew- /constexpr:steps16777216)
	add_compile_definitions(
		_CRT_SECURE_NO_DEPRECATE=1 _CRT_NON_CONFORMING_SWPRINTFS=1 _SCL_SECURE_NO_WARNINGS=1
		NOMINMAX _ENABLE_EXTENDED_ALIGNED_STORAGE=1 _HAS_EXCEPTIONS=0)
	add_link_options(/DYNAMICBASE:NO /BASE:0x10000 /FIXED)

	#TODO: Some of these could be cleaned up
	add_compile_options(/wd4805) # Comparing boolean and int
	add_compile_options(/wd4804) # Using integer operators with booleans
	add_compile_options(/wd4200) # Zero-sized array in struct/union
	add_link_options(/ignore:4281) # Undesirable base address 0x10000

	# MSVC 2017 uses iterator as base class internally, causing a lot of warning spam
	add_compile_definitions(_SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING=1)

	# Increase stack limit to 8 MB
	add_link_options(/STACK:8388608,1048576)
else()
	# Some distros have the compilers set to use PIE by default, but RPCS3 doesn't work with PIE, so we need to disable it.
	check_cxx_compiler_flag("-no-pie" HAS_NO_PIE)
	check_cxx_compiler_flag("-march=native" COMPILER_SUPPORTS_MARCH_NATIVE)
	check_cxx_compiler_flag("-msse -msse2 -mcx16" COMPILER_X86)
	if (APPLE)
		check_cxx_compiler_flag("-march=armv8.4-a" COMPILER_ARM)
	else()
		check_cxx_compiler_flag("-march=armv8.1-a" COMPILER_ARM)
	endif()

	add_compile_options(-Wall)
	add_compile_options(-fno-exceptions)
	add_compile_options(-fstack-protector)

	if (COMPILER_X86)
		add_compile_options(-msse -msse2 -mcx16)
	endif()

	if (COMPILER_ARM)
		if (APPLE)
			add_compile_options(-march=armv8.4-a)
		else()
			add_compile_options(-march=armv8.1-a)
		endif()
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

	if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
		add_compile_options(-Werror=inconsistent-missing-override)
	elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
		add_compile_options(-Werror=suggest-override)
		add_compile_options(-Wclobbered)
		# add_compile_options(-Wcast-function-type)
		add_compile_options(-Wduplicated-branches)
		add_compile_options(-Wduplicated-cond)
	endif()

	#TODO Clean the code so these are removed
	if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
		add_compile_options(-fconstexpr-steps=16777216)
		add_compile_options(-Wno-unused-lambda-capture)
		add_compile_options(-Wno-unused-private-field)
		add_compile_options(-Wno-delete-non-virtual-dtor)
		add_compile_options(-Wno-unused-command-line-argument)
	elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
		add_compile_options(-Wno-class-memaccess)
	endif()

	if(USE_NATIVE_INSTRUCTIONS AND COMPILER_SUPPORTS_MARCH_NATIVE)
		add_compile_options(-march=native)
	endif()

	if(NOT APPLE AND NOT WIN32)
		# This hides our LLVM from mesa's LLVM, otherwise we get some unresolvable conflicts.
		add_link_options(-Wl,--exclude-libs,ALL)

		if(HAS_NO_PIE)
			add_link_options(-no-pie)
		endif()
	elseif(APPLE)
		if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
			add_compile_options(-stdlib=libc++)
		endif()

		if (CMAKE_OSX_ARCHITECTURES MATCHES "x86_64")
			add_link_options(-Wl,-image_base,0x10000 -Wl,-pagezero_size,0x10000)
			add_link_options(-Wl,-no_pie)
		endif()
	elseif(WIN32)
		add_compile_definitions(__STDC_FORMAT_MACROS=1)

		# Workaround for mingw64 (MSYS2)
		add_link_options(-Wl,--allow-multiple-definition)

		# Increase stack limit to 8 MB
		add_link_options(-Wl,--stack -Wl,8388608)

		add_link_options(-Wl,--image-base,0x10000)
	endif()
endif()
