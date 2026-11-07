# The OpenXR loader overrides the runtime library itself, so we have to set it again afterwards.
# It has to match RPCS3: static in Release (MultiThreaded) and dynamic in Debug (MultiThreadedDebugDLL), see OpenXR.vcxproj.
cmake_language(DEFER CALL
    set_property
    TARGET openxr_loader
    PROPERTY
    MSVC_RUNTIME_LIBRARY
    "${CMAKE_MSVC_RUNTIME_LIBRARY}"
)
