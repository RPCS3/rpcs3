# Copy remaining DLLS

# Normalize paths before matching the exclude regexes (newer CMake versions warn otherwise)
if(POLICY CMP0207)
    cmake_policy(SET CMP0207 NEW)
endif()

cmake_path(GET exe PARENT_PATH exe_dir)
cmake_path(CONVERT "${MSYS2_CLANG_BIN}" TO_CMAKE_PATH_LIST msys2_clang_bin)
cmake_path(CONVERT "${MSYS2_USR_BIN}" TO_CMAKE_PATH_LIST msys2_usr_bin)

# Qt plugins are loaded at runtime, so their dependencies don't show up in the executable's imports
if(NOT DEFINED plugin_dir)
    set(plugin_dir "${exe_dir}/share/qt6/plugins")
endif()

file(GLOB_RECURSE plugins "${plugin_dir}/*.dll")
list(LENGTH plugins plugin_count)

set(module_args)
if(plugins)
    set(module_args MODULES ${plugins})
endif()

message(STATUS "Getting runtime dependencies for '${exe}' in '${exe_dir}' and ${plugin_count} plugins in '${plugin_dir}'")
message(STATUS "Dependency dirs: '${msys2_clang_bin}', '${msys2_usr_bin}'")

# Don't follow system DLLs. Their dependencies are API sets (api-ms-*, ext-ms-*) and Windows internals that can never be resolved.
file(GET_RUNTIME_DEPENDENCIES EXECUTABLES ${exe}
    ${module_args}
    RESOLVED_DEPENDENCIES_VAR resolved_deps
    UNRESOLVED_DEPENDENCIES_VAR unresolved_deps
    CONFLICTING_DEPENDENCIES_PREFIX conflicts
    PRE_EXCLUDE_REGEXES "^[Aa][Pp][Ii]-[Mm][Ss]-.*" "^[Ee][Xx][Tt]-[Mm][Ss]-.*"
    POST_EXCLUDE_REGEXES ".*[/\\\\][Ww][Ii][Nn][Dd][Oo][Ww][Ss][/\\\\].*"
    DIRECTORIES ${msys2_clang_bin} ${msys2_usr_bin})

# The executable finds DLLs that windeployqt copied next to it (e.g. Qt6Core.dll), but the plugins don't search
# the executable's directory and find the same DLLs in the MSYS2 dirs. Use the MSYS2 path for these, so they are
# handled like any other dependency below (copied if missing, skipped if already there).
foreach(dep_filename IN LISTS conflicts_FILENAMES)
    set(msys2_path "")

    foreach(path IN LISTS conflicts_${dep_filename})
        cmake_path(GET path PARENT_PATH path_dir_raw)
        cmake_path(CONVERT "${path_dir_raw}" TO_CMAKE_PATH_LIST path_dir)

        if(path_dir STREQUAL msys2_clang_bin OR path_dir STREQUAL msys2_usr_bin)
            set(msys2_path "${path}")
            break()
        endif()
    endforeach()

    if(msys2_path)
        list(APPEND resolved_deps "${msys2_path}")
    else()
        message(WARNING "Conflicting paths for '${dep_filename}', none of them in the MSYS2 dirs: ${conflicts_${dep_filename}}")
    endif()
endforeach()

foreach(dep IN LISTS resolved_deps)
    cmake_path(GET dep FILENAME dep_filename)
    cmake_path(GET dep PARENT_PATH dep_dir_raw)
    cmake_path(CONVERT "${dep_dir_raw}" TO_CMAKE_PATH_LIST dep_dir)

    string(COMPARE EQUAL "${dep_dir}" "${msys2_clang_bin}" is_clang_path)
    string(COMPARE EQUAL "${dep_dir}" "${msys2_usr_bin}" is_usr_path)

    set(same_path FALSE)
    if(is_clang_path OR is_usr_path)
        set(same_path TRUE)
    endif()

    if(same_path)
        set(dest "${exe_dir}/${dep_filename}")
        if(NOT EXISTS "${dest}")
            file(COPY "${dep}" DESTINATION "${exe_dir}")
            message(STATUS "Copied '${dep_filename}' to '${exe_dir}'")
        else()
            message(STATUS "Already exists: '${dest}', skipping.")
        endif()
    else()
        message(STATUS "Found and ignored '${dep}' in '${dep_dir}'. Is clang path: ${is_clang_path}, is usr path: ${is_usr_path}")
    endif()
endforeach()

# Warn about unresolved dependencies
if(unresolved_deps)
    message(WARNING "Unresolved dependencies:")
    # Unresolved dependencies are only names, system DLLs are already excluded above
    foreach(dep IN LISTS unresolved_deps)
        message(STATUS " - ${dep}")
    endforeach()
endif()
