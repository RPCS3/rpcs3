# CURL

if(USE_SYSTEM_CURL)
	message(STATUS "RPCS3: using shared libcurl")
	find_package(CURL REQUIRED)
	add_library(libcurl INTERFACE)
	target_link_libraries(libcurl INTERFACE CURL::libcurl)
else()
	message(STATUS "RPCS3: building libcurl + wolfssl submodules")
	set(BUILD_CURL_EXE OFF CACHE BOOL "Set to ON to build curl executable.")
	# If set to ON then CURL can not find our wolfssl
	set(CURL_USE_WOLFSSL OFF CACHE BOOL "enable wolfSSL for SSL/TLS")
	set(CURL_USE_OPENSSL OFF CACHE BOOL "Use OpenSSL code. Experimental")
	set(HTTP_ONLY ON CACHE BOOL "disables all protocols except HTTP (This overrides all CURL_DISABLE_* options)")
	set(USE_LIBIDN2 OFF CACHE BOOL "Use libidn2 for IDN support") # Disabled because MacOS CI doesn't work otherwise
	set(CURL_CA_PATH "none" CACHE STRING "Location of default CA path. Set 'none' to disable or 'auto' for auto-detection. Defaults to 'auto'.")
	if(USE_MSVC_STATIC_CRT)
		set(CURL_STATIC_CRT ON CACHE BOOL "Use static crt to build curl")
	endif()
	if(WIN32)
		set(ENABLE_UNICODE ON CACHE BOOL "enable Unicode")
	endif()
	set(CURL_USE_LIBSSH2 OFF CACHE BOOL "Use libSSH2")
	set(CURL_USE_LIBPSL OFF CACHE BOOL "Use libPSL")

	set(SSL_ENABLED ON)
	set(USE_WOLFSSL ON)

	add_subdirectory(curl EXCLUDE_FROM_ALL)

	target_link_libraries(libcurl PRIVATE wolfssl)

endif()
