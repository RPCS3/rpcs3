# CURL

if(USE_SYSTEM_CURL)
	message("-- RPCS3: using shared libcurl")
	find_package(CURL REQUIRED)
	add_library(libcurl INTERFACE)
	target_link_libraries(libcurl INTERFACE CURL::libcurl)
else()
	message("-- RPCS3: building libcurl + wolfssl submodules")
	add_compile_definitions(HAVE_SNI OPENSSL_EXTRA)
	option(BUILD_CURL_EXE "Set to ON to build curl executable." OFF)
	option(CURL_USE_WOLFSSL "enable wolfSSL for SSL/TLS" ON)
	option(HTTP_ONLY "disables all protocols except HTTP (This overrides all CURL_DISABLE_* options)" ON)
	option(CURL_ZLIB "Set to ON to enable building curl with zlib support." OFF)
	option(USE_LIBIDN2 "Use libidn2 for IDN support" OFF) # Disabled because MacOs CI doesn't work otherwise
	set(CURL_CA_PATH "none" CACHE STRING "Location of default CA path. Set 'none' to disable or 'auto' for auto-detection. Defaults to 'auto'.")
	if(USE_MSVC_STATIC_CRT)
		set(CURL_STATIC_CRT ON CACHE BOOL "Use static crt to build curl" FORCE)
	endif()
	add_subdirectory(curl EXCLUDE_FROM_ALL)

	set_property(TARGET libcurl PROPERTY FOLDER "3rdparty/")

endif()
