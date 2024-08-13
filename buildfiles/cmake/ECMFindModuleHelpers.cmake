# SPDX-FileCopyrightText: 2014 Alex Merry <alex.merry@kde.org>
#
# SPDX-License-Identifier: BSD-3-Clause

#[=======================================================================[.rst:
ECMFindModuleHelpers
--------------------

Helper macros for find modules: ``ecm_find_package_version_check()``,
``ecm_find_package_parse_components()`` and
``ecm_find_package_handle_library_components()``.

::

  ecm_find_package_version_check(<name>)

Prints warnings if the CMake version or the project's required CMake version
is older than that required by extra-cmake-modules.

::

  ecm_find_package_parse_components(<name>
      RESULT_VAR <variable>
      KNOWN_COMPONENTS <component1> [<component2> [...]]
      [SKIP_DEPENDENCY_HANDLING])

This macro will populate <variable> with a list of components found in
<name>_FIND_COMPONENTS, after checking that all those components are in the
list of ``KNOWN_COMPONENTS``; if there are any unknown components, it will print
an error or warning (depending on the value of <name>_FIND_REQUIRED) and call
``return()``.

The order of components in <variable> is guaranteed to match the order they
are listed in the ``KNOWN_COMPONENTS`` argument.

If ``SKIP_DEPENDENCY_HANDLING`` is not set, for each component the variable
<name>_<component>_component_deps will be checked for dependent components.
If <component> is listed in <name>_FIND_COMPONENTS, then all its (transitive)
dependencies will also be added to <variable>.

::

  ecm_find_package_handle_library_components(<name>
      COMPONENTS <component> [<component> [...]]
      [SKIP_DEPENDENCY_HANDLING])
      [SKIP_PKG_CONFIG])

Creates an imported library target for each component.  The operation of this
macro depends on the presence of a number of CMake variables.

The <name>_<component>_lib variable should contain the name of this library,
and <name>_<component>_header variable should contain the name of a header
file associated with it (whatever relative path is normally passed to
'#include'). <name>_<component>_header_subdir variable can be used to specify
which subdirectory of the include path the headers will be found in.
``ecm_find_package_components()`` will then search for the library
and include directory (creating appropriate cache variables) and create an
imported library target named <name>::<component>.

Additional variables can be used to provide additional information:

If ``SKIP_PKG_CONFIG``, the <name>_<component>_pkg_config variable is set, and
pkg-config is found, the pkg-config module given by
<name>_<component>_pkg_config will be searched for and used to help locate the
library and header file.  It will also be used to set
<name>_<component>_VERSION.

Note that if version information is found via pkg-config,
<name>_<component>_FIND_VERSION can be set to require a particular version
for each component.

If ``SKIP_DEPENDENCY_HANDLING`` is not set, the ``INTERFACE_LINK_LIBRARIES`` property
of the imported target for <component> will be set to contain the imported
targets for the components listed in <name>_<component>_component_deps.
<component>_FOUND will also be set to ``FALSE`` if any of the components in
<name>_<component>_component_deps are not found.  This requires the components
in <name>_<component>_component_deps to be listed before <component> in the
``COMPONENTS`` argument.

The following variables will be set:

``<name>_TARGETS``
  the imported targets
``<name>_LIBRARIES``
  the found libraries
``<name>_INCLUDE_DIRS``
  the combined required include directories for the components
``<name>_DEFINITIONS``
  the "other" CFLAGS provided by pkg-config, if any
``<name>_VERSION``
  the value of ``<name>_<component>_VERSION`` for the first component that
  has this variable set (note that components are searched for in the order
  they are passed to the macro), although if it is already set, it will not
  be altered

.. note::
  These variables are never cleared, so if
  ``ecm_find_package_handle_library_components()`` is called multiple times with
  different components (typically because of multiple ``find_package()`` calls) then
  ``<name>_TARGETS``, for example, will contain all the targets found in any
  call (although no duplicates).

Since pre-1.0.0.
#]=======================================================================]

macro(ecm_find_package_version_check module_name)
    if(CMAKE_VERSION VERSION_LESS 3.16.0)
        message(FATAL_ERROR "CMake 3.16.0 is required by Find${module_name}.cmake")
    endif()
    if(CMAKE_MINIMUM_REQUIRED_VERSION VERSION_LESS 3.16.0)
        message(AUTHOR_WARNING "Your project should require at least CMake 3.16.0 to use Find${module_name}.cmake")
    endif()
endmacro()

macro(ecm_find_package_parse_components module_name)
    set(ecm_fppc_options SKIP_DEPENDENCY_HANDLING)
    set(ecm_fppc_oneValueArgs RESULT_VAR)
    set(ecm_fppc_multiValueArgs KNOWN_COMPONENTS DEFAULT_COMPONENTS)
    cmake_parse_arguments(ECM_FPPC "${ecm_fppc_options}" "${ecm_fppc_oneValueArgs}" "${ecm_fppc_multiValueArgs}" ${ARGN})

    if(ECM_FPPC_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unexpected arguments to ecm_find_package_parse_components: ${ECM_FPPC_UNPARSED_ARGUMENTS}")
    endif()
    if(NOT ECM_FPPC_RESULT_VAR)
        message(FATAL_ERROR "Missing RESULT_VAR argument to ecm_find_package_parse_components")
    endif()
    if(NOT ECM_FPPC_KNOWN_COMPONENTS)
        message(FATAL_ERROR "Missing KNOWN_COMPONENTS argument to ecm_find_package_parse_components")
    endif()
    if(NOT ECM_FPPC_DEFAULT_COMPONENTS)
        set(ECM_FPPC_DEFAULT_COMPONENTS ${ECM_FPPC_KNOWN_COMPONENTS})
    endif()

    if(${module_name}_FIND_COMPONENTS)
        set(ecm_fppc_requestedComps ${${module_name}_FIND_COMPONENTS})

        if(NOT ECM_FPPC_SKIP_DEPENDENCY_HANDLING)
            # Make sure deps are included
            foreach(ecm_fppc_comp ${ecm_fppc_requestedComps})
                foreach(ecm_fppc_dep_comp ${${module_name}_${ecm_fppc_comp}_component_deps})
                    list(FIND ecm_fppc_requestedComps "${ecm_fppc_dep_comp}" ecm_fppc_index)
                    if("${ecm_fppc_index}" STREQUAL "-1")
                        if(NOT ${module_name}_FIND_QUIETLY)
                            message(STATUS "${module_name}: ${ecm_fppc_comp} requires ${${module_name}_${ecm_fppc_comp}_component_deps}")
                        endif()
                        list(APPEND ecm_fppc_requestedComps "${ecm_fppc_dep_comp}")
                    endif()
                endforeach()
            endforeach()
        else()
            message(STATUS "Skipping dependency handling for ${module_name}")
        endif()
        list(REMOVE_DUPLICATES ecm_fppc_requestedComps)

        # This makes sure components are listed in the same order as
        # KNOWN_COMPONENTS (potentially important for inter-dependencies)
        set(${ECM_FPPC_RESULT_VAR})
        foreach(ecm_fppc_comp ${ECM_FPPC_KNOWN_COMPONENTS})
            list(FIND ecm_fppc_requestedComps "${ecm_fppc_comp}" ecm_fppc_index)
            if(NOT "${ecm_fppc_index}" STREQUAL "-1")
                list(APPEND ${ECM_FPPC_RESULT_VAR} "${ecm_fppc_comp}")
                list(REMOVE_AT ecm_fppc_requestedComps ${ecm_fppc_index})
            endif()
        endforeach()
        # if there are any left, they are unknown components
        if(ecm_fppc_requestedComps)
            set(ecm_fppc_msgType STATUS)
            if(${module_name}_FIND_REQUIRED)
                set(ecm_fppc_msgType FATAL_ERROR)
            endif()
            if(NOT ${module_name}_FIND_QUIETLY)
                message(${ecm_fppc_msgType} "${module_name}: requested unknown components ${ecm_fppc_requestedComps}")
            endif()
            return()
        endif()
    else()
        set(${ECM_FPPC_RESULT_VAR} ${ECM_FPPC_DEFAULT_COMPONENTS})
    endif()
endmacro()

macro(ecm_find_package_handle_library_components module_name)
    set(ecm_fpwc_options SKIP_PKG_CONFIG SKIP_DEPENDENCY_HANDLING)
    set(ecm_fpwc_oneValueArgs)
    set(ecm_fpwc_multiValueArgs COMPONENTS)
    cmake_parse_arguments(ECM_FPWC "${ecm_fpwc_options}" "${ecm_fpwc_oneValueArgs}" "${ecm_fpwc_multiValueArgs}" ${ARGN})

    if(ECM_FPWC_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unexpected arguments to ecm_find_package_handle_components: ${ECM_FPWC_UNPARSED_ARGUMENTS}")
    endif()
    if(NOT ECM_FPWC_COMPONENTS)
        message(FATAL_ERROR "Missing COMPONENTS argument to ecm_find_package_handle_components")
    endif()

    include(FindPackageHandleStandardArgs)
    find_package(PkgConfig QUIET)
    foreach(ecm_fpwc_comp ${ECM_FPWC_COMPONENTS})
        set(ecm_fpwc_dep_vars)
        set(ecm_fpwc_dep_targets)
        if(NOT SKIP_DEPENDENCY_HANDLING)
            foreach(ecm_fpwc_dep ${${module_name}_${ecm_fpwc_comp}_component_deps})
                list(APPEND ecm_fpwc_dep_vars "${module_name}_${ecm_fpwc_dep}_FOUND")
                list(APPEND ecm_fpwc_dep_targets "${module_name}::${ecm_fpwc_dep}")
            endforeach()
        endif()

        if(NOT ECM_FPWC_SKIP_PKG_CONFIG AND ${module_name}_${ecm_fpwc_comp}_pkg_config)
            pkg_check_modules(PKG_${module_name}_${ecm_fpwc_comp} QUIET
                              ${${module_name}_${ecm_fpwc_comp}_pkg_config})
        endif()

        find_path(${module_name}_${ecm_fpwc_comp}_INCLUDE_DIR
            NAMES ${${module_name}_${ecm_fpwc_comp}_header}
            HINTS ${PKG_${module_name}_${ecm_fpwc_comp}_INCLUDE_DIRS}
            PATH_SUFFIXES ${${module_name}_${ecm_fpwc_comp}_header_subdir}
        )
        find_library(${module_name}_${ecm_fpwc_comp}_LIBRARY
            NAMES ${${module_name}_${ecm_fpwc_comp}_lib}
            HINTS ${PKG_${module_name}_${ecm_fpwc_comp}_LIBRARY_DIRS}
        )

        set(${module_name}_${ecm_fpwc_comp}_VERSION "${PKG_${module_name}_${ecm_fpwc_comp}_VERSION}")
        if(NOT ${module_name}_VERSION)
            set(${module_name}_VERSION ${${module_name}_${ecm_fpwc_comp}_VERSION})
        endif()

        set(FPHSA_NAME_MISMATCHED 1)
        find_package_handle_standard_args(${module_name}_${ecm_fpwc_comp}
            FOUND_VAR
                ${module_name}_${ecm_fpwc_comp}_FOUND
            REQUIRED_VARS
                ${module_name}_${ecm_fpwc_comp}_LIBRARY
                ${module_name}_${ecm_fpwc_comp}_INCLUDE_DIR
                ${ecm_fpwc_dep_vars}
            VERSION_VAR
                ${module_name}_${ecm_fpwc_comp}_VERSION
            )
        unset(FPHSA_NAME_MISMATCHED)

        mark_as_advanced(
            ${module_name}_${ecm_fpwc_comp}_LIBRARY
            ${module_name}_${ecm_fpwc_comp}_INCLUDE_DIR
        )

        if(${module_name}_${ecm_fpwc_comp}_FOUND)
            list(APPEND ${module_name}_LIBRARIES
                        "${${module_name}_${ecm_fpwc_comp}_LIBRARY}")
            list(APPEND ${module_name}_INCLUDE_DIRS
                        "${${module_name}_${ecm_fpwc_comp}_INCLUDE_DIR}")
            set(${module_name}_DEFINITIONS
                    ${${module_name}_DEFINITIONS}
                    ${PKG_${module_name}_${ecm_fpwc_comp}_DEFINITIONS})
            if(NOT TARGET ${module_name}::${ecm_fpwc_comp})
                add_library(${module_name}::${ecm_fpwc_comp} UNKNOWN IMPORTED)
                set_target_properties(${module_name}::${ecm_fpwc_comp} PROPERTIES
                    IMPORTED_LOCATION "${${module_name}_${ecm_fpwc_comp}_LIBRARY}"
                    INTERFACE_COMPILE_OPTIONS "${PKG_${module_name}_${ecm_fpwc_comp}_DEFINITIONS}"
                    INTERFACE_INCLUDE_DIRECTORIES "${${module_name}_${ecm_fpwc_comp}_INCLUDE_DIR}"
                    INTERFACE_LINK_LIBRARIES "${ecm_fpwc_dep_targets}"
                )
            endif()
            list(APPEND ${module_name}_TARGETS
                        "${module_name}::${ecm_fpwc_comp}")
        endif()
    endforeach()
    if(${module_name}_LIBRARIES)
        list(REMOVE_DUPLICATES ${module_name}_LIBRARIES)
    endif()
    if(${module_name}_INCLUDE_DIRS)
        list(REMOVE_DUPLICATES ${module_name}_INCLUDE_DIRS)
    endif()
    if(${module_name}_DEFINITIONS)
        list(REMOVE_DUPLICATES ${module_name}_DEFINITIONS)
    endif()
    if(${module_name}_TARGETS)
        list(REMOVE_DUPLICATES ${module_name}_TARGETS)
    endif()
endmacro()
