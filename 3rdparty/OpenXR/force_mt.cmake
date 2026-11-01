cmake_language(DEFER CALL
    set_property
    TARGET openxr_loader
    PROPERTY
    MSVC_RUNTIME_LIBRARY
    "MultiThreaded$<$<CONFIG:Debug>:Debug>"
)
