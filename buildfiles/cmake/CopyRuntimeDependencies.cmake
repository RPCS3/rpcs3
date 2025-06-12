# Copy remaining DLLS

cmake_path(GET exe PARENT_PATH exe_dir)
cmake_path(CONVERT "${MSYS2_CLANG_BIN}" TO_CMAKE_PATH_LIST msys2_clang_bin)
cmake_path(CONVERT "${MSYS2_USR_BIN}" TO_CMAKE_PATH_LIST msys2_usr_bin)

message(STATUS "Getting runtime dependencies for '${exe}' in '${exe_dir}'")
message(STATUS "Dependency dirs: '${msys2_clang_bin}', '${msys2_usr_bin}'")

file(GET_RUNTIME_DEPENDENCIES EXECUTABLES ${exe}
    RESOLVED_DEPENDENCIES_VAR resolved_deps
    UNRESOLVED_DEPENDENCIES_VAR unresolved_deps
    DIRECTORIES ${msys2_clang_bin} ${msys2_usr_bin})

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
    foreach(dep IN LISTS unresolved_deps)
        cmake_path(GET dep PARENT_PATH dep_dir)
        string(TOLOWER "${dep_dir}" dep_dir_lower)
        if(NOT dep_dir_lower MATCHES ".*(windows[/\\]system32|windows[/\\]winsxs|program files).*")
            message(STATUS " - ${dep_dir}/${dep}")
        endif()
    endforeach()
endif()
