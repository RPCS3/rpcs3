# Copyright (C) 2020 The Qt Company Ltd.
# Copyright 2005-2011 Kitware, Inc.
# SPDX-License-Identifier: BSD-3-Clause

include(CMakeParseArguments)

function(qt6_create_translation _qm_files)
    set(options)
    set(oneValueArgs)
    set(multiValueArgs OPTIONS)

    cmake_parse_arguments(_LUPDATE "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    set(_lupdate_files ${_LUPDATE_UNPARSED_ARGUMENTS})
    set(_lupdate_options ${_LUPDATE_OPTIONS})

    list(FIND _lupdate_options "-extensions" _extensions_index)
    if(_extensions_index GREATER -1)
        math(EXPR _extensions_index "${_extensions_index} + 1")
        list(GET _lupdate_options ${_extensions_index} _extensions_list)
        string(REPLACE "," ";" _extensions_list "${_extensions_list}")
        list(TRANSFORM _extensions_list STRIP)
        list(TRANSFORM _extensions_list REPLACE "^\\." "")
        list(TRANSFORM _extensions_list PREPEND "*.")
    else()
        set(_extensions_list "*.java;*.jui;*.ui;*.c;*.c++;*.cc;*.cpp;*.cxx;*.ch;*.h;*.h++;*.hh;*.hpp;*.hxx;*.js;*.qs;*.qml;*.qrc")
    endif()
    set(_my_sources)
    set(_my_tsfiles)
    foreach(_file ${_lupdate_files})
        get_filename_component(_ext ${_file} EXT)
        get_filename_component(_abs_FILE ${_file} ABSOLUTE)
        if(_ext MATCHES "ts")
            list(APPEND _my_tsfiles ${_abs_FILE})
        else()
            list(APPEND _my_sources ${_abs_FILE})
        endif()
    endforeach()
    set(stamp_file_dir "${CMAKE_CURRENT_BINARY_DIR}/.lupdate")
    if(NOT EXISTS "${stamp_file_dir}")
        file(MAKE_DIRECTORY "${stamp_file_dir}")
    endif()
    set(stamp_files "")
    foreach(_ts_file ${_my_tsfiles})
        get_filename_component(_ts_name ${_ts_file} NAME)
        if(_my_sources)
          # make a list file to call lupdate on, so we don't make our commands too
          # long for some systems
          set(_ts_lst_file "${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/${_ts_name}_lst_file")
          set(_lst_file_srcs)
          set(_dependencies)
          foreach(_lst_file_src ${_my_sources})
              set(_lst_file_srcs "${_lst_file_src}\n${_lst_file_srcs}")
              if(IS_DIRECTORY ${_lst_file_src})
                  list(TRANSFORM _extensions_list PREPEND "${_lst_file_src}/" OUTPUT_VARIABLE _directory_glob)
                  file(GLOB_RECURSE _directory_contents CONFIGURE_DEPENDS ${_directory_glob})
                  list(APPEND _dependencies ${_directory_contents})
              else()
                  list(APPEND _dependencies "${_lst_file_src}")
              endif()
          endforeach()

          get_directory_property(_inc_DIRS INCLUDE_DIRECTORIES)
          foreach(_pro_include ${_inc_DIRS})
              get_filename_component(_abs_include "${_pro_include}" ABSOLUTE)
              set(_lst_file_srcs "-I${_pro_include}\n${_lst_file_srcs}")
          endforeach()

          file(WRITE ${_ts_lst_file} "${_lst_file_srcs}")
        endif()
        file(RELATIVE_PATH _ts_relative_path ${CMAKE_CURRENT_SOURCE_DIR} ${_ts_file})
        string(REPLACE "../" "__/" _ts_relative_path "${_ts_relative_path}")
        set(stamp_file "${stamp_file_dir}/${_ts_relative_path}.stamp")
        list(APPEND stamp_files ${stamp_file})
        get_filename_component(full_stamp_file_dir "${stamp_file}" DIRECTORY)
        if(NOT EXISTS "${full_stamp_file_dir}")
            file(MAKE_DIRECTORY "${full_stamp_file_dir}")
        endif()
        add_custom_command(OUTPUT ${stamp_file}
            COMMAND ${QT_CMAKE_EXPORT_NAMESPACE}::lupdate
            ARGS ${_lupdate_options} "@${_ts_lst_file}" -ts ${_ts_file}
            COMMAND ${CMAKE_COMMAND} -E touch "${stamp_file}"
            DEPENDS ${_dependencies}
            VERBATIM)
    endforeach()
    qt6_add_translation(${_qm_files} ${_my_tsfiles} __QT_INTERNAL_TIMESTAMP_FILES ${stamp_files})
    set(${_qm_files} ${${_qm_files}} PARENT_SCOPE)
endfunction()

function(qt6_add_translation _qm_files)
    set(options)
    set(oneValueArgs)
    set(multiValueArgs OPTIONS __QT_INTERNAL_TIMESTAMP_FILES)

    cmake_parse_arguments(_LRELEASE "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    set(_lrelease_files ${_LRELEASE_UNPARSED_ARGUMENTS})

    set(idx 0)
    foreach(_current_FILE ${_lrelease_files})
        get_filename_component(_abs_FILE ${_current_FILE} ABSOLUTE)
        get_filename_component(qm ${_abs_FILE} NAME)
        # everything before the last dot has to be considered the file name (including other dots)
        string(REGEX REPLACE "\\.[^.]*$" "" FILE_NAME ${qm})
        get_source_file_property(output_location ${_abs_FILE} OUTPUT_LOCATION)
        if(output_location)
            file(MAKE_DIRECTORY "${output_location}")
            set(qm "${output_location}/${FILE_NAME}.qm")
        else()
            set(qm "${CMAKE_CURRENT_BINARY_DIR}/${FILE_NAME}.qm")
        endif()

        if(_LRELEASE___QT_INTERNAL_TIMESTAMP_FILES)
            list(GET _LRELEASE___QT_INTERNAL_TIMESTAMP_FILES ${idx} qm_dep)
            math(EXPR idx "${idx} + 1")
        else()
            set(qm_dep "${_abs_FILE}")
        endif()

        add_custom_command(OUTPUT ${qm}
            COMMAND ${QT_CMAKE_EXPORT_NAMESPACE}::lrelease
            ARGS ${_LRELEASE_OPTIONS} ${_abs_FILE} -qm ${qm}
            DEPENDS ${qm_dep} VERBATIM
        )
        list(APPEND ${_qm_files} ${qm})
    endforeach()
    set(${_qm_files} ${${_qm_files}} PARENT_SCOPE)
endfunction()

function(_qt_internal_collect_translation_source_targets out_var dir)
    set(result "")
    get_property(excluded DIRECTORY "${dir}" PROPERTY QT_EXCLUDE_FROM_TRANSLATION)
    if(NOT excluded)
        get_property(subdirs DIRECTORY "${dir}" PROPERTY SUBDIRECTORIES)
        foreach(subdir IN LISTS subdirs)
            _qt_internal_collect_translation_source_targets(subresult "${subdir}")
            list(APPEND result ${subresult})
        endforeach()
        get_property(dir_targets DIRECTORY "${dir}" PROPERTY BUILDSYSTEM_TARGETS)
        foreach(target IN LISTS dir_targets)
            get_target_property(target_type ${target} TYPE)
            if(CMAKE_VERSION VERSION_LESS "3.19" AND target_type STREQUAL "INTERFACE_LIBRARY")
                # Skip INTERFACE libraries with CMake < 3.19 to avoid an error about
                # QT_EXCLUDE_FROM_TRANSLATION not being whitelisted.
                continue()
            endif()
            get_target_property(excluded ${target} QT_EXCLUDE_FROM_TRANSLATION)
            if(NOT excluded AND NOT target_type STREQUAL "UTILITY")
                list(APPEND result ${target})
            endif()
        endforeach()
    endif()
    set("${out_var}" "${result}" PARENT_SCOPE)
endfunction()

function(qt6_collect_translation_source_targets out_var)
    set(no_value_options "")
    set(single_value_options DIRECTORY)
    set(multi_value_options "")
    cmake_parse_arguments(PARSE_ARGV 1 arg
        "${no_value_options}" "${single_value_options}" "${multi_value_options}"
    )

    set(dir "${arg_DIRECTORY}")
    if(dir STREQUAL "")
        set(dir "${CMAKE_CURRENT_SOURCE_DIR}")
    endif()

    _qt_internal_collect_translation_source_targets(result "${dir}")
    set("${out_var}" "${result}" PARENT_SCOPE)
endfunction()

# Makes the paths in the unparsed arguments absolute and stores them in out_var.
function(qt_internal_make_paths_absolute out_var)
    set(result "")
    foreach(path IN LISTS ARGN)
        get_filename_component(abs_path "${path}" ABSOLUTE)
        list(APPEND result "${abs_path}")
    endforeach()
    set("${out_var}" "${result}" PARENT_SCOPE)
endfunction()

# If the given TS_FILE does not exist, write an initial .ts file that can be read by lrelease and
# updated by lupdate.
function(_qt_internal_ensure_ts_file)
    set(no_value_options "")
    set(single_value_options TS_FILE)
    set(multi_value_options "")
    cmake_parse_arguments(PARSE_ARGV 0 arg
        "${no_value_options}" "${single_value_options}" "${multi_value_options}"
    )

    if(EXISTS "${arg_TS_FILE}")
        return()
    endif()

    set(content
        [[<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1"]])

    set(scope_args "")
    if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.18")
        set(scope_args DIRECTORY ${PROJECT_SOURCE_DIR})
    endif()
    get_property(language SOURCE "${arg_TS_FILE}" ${scope_args} PROPERTY
        _qt_i18n_translated_language)
    if(NOT "${language}" STREQUAL "")
        string(APPEND content " language=\"${language}\"")
    endif()

    if(NOT "${QT_I18N_SOURCE_LANGUAGE}" STREQUAL "")
        string(APPEND content " sourcelanguage=\"${QT_I18N_SOURCE_LANGUAGE}\"")
    endif()
    string(APPEND content "/>\n")
    file(WRITE "${arg_TS_FILE}" "${content}")
endfunction()

# Needed to locate Qt6LupdateProject.json.in file inside functions
set(_Qt6_LINGUIST_TOOLS_DIR ${CMAKE_CURRENT_LIST_DIR} CACHE INTERNAL "")

function(qt6_add_lupdate)
    set(options
        NO_GLOBAL_TARGET)
    set(oneValueArgs
        PLURALS_TS_FILE
        LUPDATE_TARGET)
    set(multiValueArgs
        SOURCE_TARGETS
        TS_FILES
        SOURCES
        INCLUDE_DIRECTORIES
        OPTIONS)
    cmake_parse_arguments(arg "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Set up the list of targets. Support the old command signature that takes one target as first
    # argument.
    set(targets "${arg_SOURCE_TARGETS}")
    if("${targets}" STREQUAL "")
        list(POP_FRONT arg_UNPARSED_ARGUMENTS target)
        if(TARGET "${target}")
            list(APPEND targets ${target})
        endif()
        unset(target)
    endif()

    if("${targets}" STREQUAL "" AND "${arg_SOURCES}" STREQUAL "")
        message(FATAL_ERROR "No SOURCE_TARGETS nor SOURCES were given.")
    endif()

    # Set up the name of the custom target.
    set(lupdate_target "${arg_LUPDATE_TARGET}")
    if("${lupdate_target}" STREQUAL "")
        set(lupdate_target "${PROJECT_NAME}_lupdate")
        set(lupdate_target_orig "${lupdate_target}")
        set(n 1)
        while(TARGET "${lupdate_target}")
            set(lupdate_target "${lupdate_target_orig}${n}")
            math(EXPR n "${n} + 1")
        endwhile()
    endif()

    set(includePaths "")
    set(sources "")
    list(LENGTH targets targets_length)

    if(arg_INCLUDE_DIRECTORIES)
        qt_internal_make_paths_absolute(additionalIncludePaths "${arg_INCLUDE_DIRECTORIES}")
    endif()
    if(arg_SOURCES)
        qt_internal_make_paths_absolute(additionalSources "${arg_SOURCES}")
    endif()

    set(lupdate_work_dir "${CMAKE_CURRENT_BINARY_DIR}/.lupdate")
    qt_internal_make_paths_absolute(ts_files "${arg_TS_FILES}")
    set(plurals_ts_file "")
    set(raw_plurals_ts_file "")
    if(NOT "${arg_PLURALS_TS_FILE}" STREQUAL "")
        qt_internal_make_paths_absolute(plurals_ts_file "${arg_PLURALS_TS_FILE}")
        _qt_internal_ensure_ts_file(TS_FILE "${plurals_ts_file}")
        get_filename_component(raw_plurals_ts_file "${plurals_ts_file}" NAME)
        string(PREPEND raw_plurals_ts_file "${lupdate_work_dir}/")
        list(APPEND ts_files "${raw_plurals_ts_file}")
    endif()

    set(lupdate_project_base "${lupdate_work_dir}/${lupdate_target}_project")
    set(lupdate_project_cmake "${lupdate_project_base}")
    get_property(multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
    if(multi_config)
        string(APPEND lupdate_project_cmake ".$<CONFIG>")
    endif()
    string(APPEND lupdate_project_cmake ".cmake")
    set(lupdate_project_json "${lupdate_project_base}.json")
    set(content "set(lupdate_project_file \"${CMAKE_CURRENT_LIST_FILE}\")
set(lupdate_translations \"${ts_files}\")
set(lupdate_include_paths \"${additionalIncludePaths}\")
set(lupdate_sources \"${additionalSources}\")
set(lupdate_subproject_count ${targets_length})
")
    set(exclude_ts "\\.ts$")
    set(n 1)
    foreach(target IN LISTS targets)
        set(includePaths "$<TARGET_PROPERTY:${target},INCLUDE_DIRECTORIES>")
        set(sources "$<FILTER:$<TARGET_PROPERTY:${target},SOURCES>,EXCLUDE,${exclude_ts}>")
        set(excluded "$<TARGET_PROPERTY:${target},QT_EXCLUDE_SOURCES_FROM_TRANSLATION>")
        set(autogen_build_dir_genex "$<TARGET_PROPERTY:${target},AUTOGEN_BUILD_DIR>")
        set(default_autogen_build_dir "$<TARGET_PROPERTY:${target},BINARY_DIR>/${target}_autogen")
        set(autogen_dir "$<IF:$<BOOL:${autogen_build_dir_genex}>,${autogen_build_dir_genex},${default_autogen_build_dir}>")
        string(APPEND content "
set(lupdate_subproject${n}_source_dir \"$<TARGET_PROPERTY:${target},SOURCE_DIR>\")
set(lupdate_subproject${n}_include_paths \"${includePaths}\")
set(lupdate_subproject${n}_sources \"${sources}\")
set(lupdate_subproject${n}_excluded \"${excluded}\")
set(lupdate_subproject${n}_autogen_dir \"${autogen_dir}\")
")
        math(EXPR n "${n} + 1")
    endforeach()
    file(GENERATE OUTPUT "${lupdate_project_cmake}" CONTENT "${content}")

    _qt_internal_get_tool_wrapper_script_path(tool_wrapper)
    set(lupdate_command
        COMMAND
            "${tool_wrapper}"
            $<TARGET_FILE:${QT_CMAKE_EXPORT_NAMESPACE}::lupdate>)
    set(prepare_native_ts_command "")
    set(finish_native_ts_command "")
    if(NOT plurals_ts_file STREQUAL "")
        # Copy the existing .ts file to preserve already translated strings.
        set(prepare_native_ts_command
            COMMAND
            "${CMAKE_COMMAND}" -E copy "${plurals_ts_file}" "${raw_plurals_ts_file}"
        )

        # Filter out the non-numerus forms with lconvert.
        set(finish_native_ts_command
            COMMAND
            "${tool_wrapper}"
            $<TARGET_FILE:${QT_CMAKE_EXPORT_NAMESPACE}::lconvert>
            -pluralonly
            -i "${raw_plurals_ts_file}"
            -o "${plurals_ts_file}"
        )
    endif()
    add_custom_target(${lupdate_target}
        COMMAND "${CMAKE_COMMAND}" "-DIN_FILE=${lupdate_project_cmake}"
                "-DOUT_FILE=${lupdate_project_json}"
                -P "${_Qt6_LINGUIST_TOOLS_DIR}/GenerateLUpdateProject.cmake"
        ${prepare_native_ts_command}
        ${lupdate_command} -project "${lupdate_project_json}" ${arg_OPTIONS}
        ${finish_native_ts_command}
        DEPENDS ${QT_CMAKE_EXPORT_NAMESPACE}::lupdate
        VERBATIM)

    if(NOT DEFINED QT_GLOBAL_LUPDATE_TARGET)
        set(QT_GLOBAL_LUPDATE_TARGET update_translations)
    endif()

    if(NOT arg_NO_GLOBAL_TARGET)
        _qt_internal_add_phony_target(${QT_GLOBAL_LUPDATE_TARGET}
            WARNING_VARIABLE QT_NO_GLOBAL_LUPDATE_TARGET_CREATION_WARNING
        )
        _qt_internal_add_phony_target_dependencies(${QT_GLOBAL_LUPDATE_TARGET}
            ${lupdate_target}
        )
    endif()
endfunction()

function(_qt_internal_store_languages_from_ts_files_in_targets targets ts_files)
    if(NOT APPLE)
        return()
    endif()
    set(supported_languages "")
    foreach(ts_file IN LISTS ts_files)
        execute_process(COMMAND /usr/bin/xmllint --xpath "string(/TS/@language)" ${ts_file}
            OUTPUT_STRIP_TRAILING_WHITESPACE
            OUTPUT_VARIABLE language_code
            ERROR_VARIABLE xmllint_error)
        if(NOT language_code OR xmllint_error)
            message(WARNING "Failed to resolve language code for ${ts_file}. "
                "Please update CFBundleLocalizations in your Info.plist manually.")
        endif()
    endforeach()
    foreach(target IN LISTS targets)
        set_property(TARGET "${target}" APPEND PROPERTY
            _qt_apple_supported_languages "${supported_languages}"
        )
    endforeach()
endfunction()

# Store in ${out_var} the file path to the .qm file that will be generated from the given .ts file.
function(_qt_internal_generated_qm_file_path out_var ts_file default_out_dir)
    get_filename_component(qm ${ts_file} NAME_WLE)
    string(APPEND qm ".qm")
    get_source_file_property(output_location ${ts_file} OUTPUT_LOCATION)
    if(output_location)
        if(NOT IS_ABSOLUTE "${output_location}")
            get_filename_component(output_location "${output_location}" ABSOLUTE
                BASE_DIR "${default_out_dir}")
        endif()
        string(PREPEND qm "${output_location}/")
    else()
        string(PREPEND qm "${default_out_dir}/")
    endif()
    set("${out_var}" "${qm}" PARENT_SCOPE)
endfunction()

function(qt6_add_lrelease)
    set(options
        NO_TARGET_DEPENDENCY        ### Qt7: remove together with legacy signature
        EXCLUDE_FROM_ALL
        NO_GLOBAL_TARGET)
    set(oneValueArgs
        __QT_INTERNAL_DEFAULT_QM_OUT_DIR
        LRELEASE_TARGET
        QM_FILES_OUTPUT_VARIABLE)
    set(multiValueArgs
        TS_FILES
        OPTIONS)
    cmake_parse_arguments(arg "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    qt_internal_make_paths_absolute(ts_files "${arg_TS_FILES}")

    # Support the old command signature that takes one target as first argument.
    set(legacy_signature_used FALSE)
    set(legacy_target "")
    list(LENGTH arg_UNPARSED_ARGUMENTS unparsed_arguments_count)
    if(unparsed_arguments_count GREATER 0)
        list(POP_FRONT arg_UNPARSED_ARGUMENTS legacy_target)
        if(TARGET "${legacy_target}")
            set(legacy_signature_used TRUE)
        else()
            set(legacy_target "")
        endif()
    endif()

    # Set up the driving target.
    set(lrelease_target "${arg_LRELEASE_TARGET}")
    if("${lrelease_target}" STREQUAL "")
        set(lrelease_target "${PROJECT_NAME}_lrelease")
        set(lrelease_target_orig "${lrelease_target}")
        set(n 1)
        while(TARGET "${lrelease_target}")
            set(lrelease_target "${lrelease_target_orig}${n}")
            math(EXPR n "${n} + 1")
        endwhile()
    endif()

    _qt_internal_get_tool_wrapper_script_path(tool_wrapper)
    set(lrelease_command
        COMMAND
            "${tool_wrapper}"
            $<TARGET_FILE:${QT_CMAKE_EXPORT_NAMESPACE}::lrelease>)

    set(default_qm_out_dir "${CMAKE_CURRENT_BINARY_DIR}")
    if(NOT "${arg___QT_INTERNAL_DEFAULT_QM_OUT_DIR}" STREQUAL "")
        set(default_qm_out_dir "${arg___QT_INTERNAL_DEFAULT_QM_OUT_DIR}")
    endif()

    set(qm_files "")
    foreach(ts_file ${ts_files})
        if(NOT EXISTS "${ts_file}")
            message(WARNING "Translation file '${ts_file}' does not exist. "
                "Consider building the target 'update_translations' to create an initial "
                "version of that file.")
            _qt_internal_ensure_ts_file(TS_FILE "${ts_file}")
        endif()

        _qt_internal_generated_qm_file_path(qm "${ts_file}" "${default_qm_out_dir}")
        get_filename_component(qm_dir "${qm}" DIRECTORY)
        add_custom_command(OUTPUT ${qm}
            COMMAND "${CMAKE_COMMAND}" -E make_directory "${qm_dir}"
            ${lrelease_command} ${arg_OPTIONS} ${ts_file} -qm ${qm}
            DEPENDS ${QT_CMAKE_EXPORT_NAMESPACE}::lrelease "${ts_file}"
            VERBATIM)
        list(APPEND qm_files "${qm}")

        # QTBUG-103470: Save the target responsible for driving the build of the custom command
        # into an internal source file property. It will be added as a dependency for targets
        # created by _qt_internal_process_resource, to avoid the Xcode issue of not allowing
        # multiple targets depending on the output, without having a common target ancestor.
        set(scope_args "")
        if(legacy_signature_used AND CMAKE_VERSION VERSION_GREATER_EQUAL "3.18")
            set(scope_args TARGET_DIRECTORY ${legacy_target})
        endif()
        set_source_files_properties("${qm}" ${scope_args} PROPERTIES
            _qt_resource_target_dependency "${lrelease_target}"
        )
    endforeach()

    if(legacy_signature_used)
        _qt_internal_store_languages_from_ts_files_in_targets("${legacy_target}" "${ts_files}")
    endif()

    if(legacy_signature_used)
        add_custom_target(${lrelease_target} DEPENDS ${qm_files})
        if(NOT arg_NO_TARGET_DEPENDENCY)
            add_dependencies(${legacy_target} ${lrelease_target})
        endif()
    else()
        set(maybe_all ALL)
        if(arg_EXCLUDE_FROM_ALL)
            set(maybe_all "")
        endif()
        add_custom_target(${lrelease_target} ${maybe_all} DEPENDS ${qm_files})
    endif()

    if(NOT DEFINED QT_GLOBAL_LRELEASE_TARGET)
        set(QT_GLOBAL_LRELEASE_TARGET release_translations)
    endif()

    if(NOT arg_NO_GLOBAL_TARGET)
        if(NOT TARGET ${QT_GLOBAL_LRELEASE_TARGET})
            add_custom_target(${QT_GLOBAL_LRELEASE_TARGET})
        endif()
        add_dependencies(${QT_GLOBAL_LRELEASE_TARGET} ${lrelease_target})
    endif()

    if(NOT "${arg_QM_FILES_OUTPUT_VARIABLE}" STREQUAL "")
        set("${arg_QM_FILES_OUTPUT_VARIABLE}" "${qm_files}" PARENT_SCOPE)
    endif()
endfunction()

function(qt6_add_translations)
    set(options
        IMMEDIATE_CALL
        NO_GENERATE_PLURALS_TS_FILE)
    set(oneValueArgs
        __QT_INTERNAL_DEFAULT_QM_OUT_DIR
        LUPDATE_TARGET
        LRELEASE_TARGET
        TS_FILES_OUTPUT_VARIABLE
        QM_FILES_OUTPUT_VARIABLE
        RESOURCE_PREFIX
        OUTPUT_TARGETS)
    set(multiValueArgs
        TARGETS
        SOURCE_TARGETS
        TS_FILES
        TS_FILE_BASE
        TS_FILE_DIR
        PLURALS_TS_FILE
        SOURCES
        INCLUDE_DIRECTORIES
        LUPDATE_OPTIONS
        LRELEASE_OPTIONS)
    cmake_parse_arguments(arg "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(targets "${arg_TARGETS}")
    if(NOT "${arg_UNPARSED_ARGUMENTS}" STREQUAL "")
        list(POP_FRONT arg_UNPARSED_ARGUMENTS target)
        list(PREPEND targets "${target}")
        unset(target)
        set(arg_TARGETS ${targets})       # to forward this argument
    endif()
    if(targets STREQUAL "")
        message(FATAL_ERROR "No targets provided.")
    endif()
    if(DEFINED arg_RESOURCE_PREFIX AND DEFINED arg_QM_FILES_OUTPUT_VARIABLE)
        message(FATAL_ERROR "QM_FILES_OUTPUT_VARIABLE cannot be specified "
            "together with RESOURCE_PREFIX.")
    endif()
    if(DEFINED arg_QM_FILES_OUTPUT_VARIABLE AND DEFINED arg_OUTPUT_TARGETS)
        message(FATAL_ERROR "OUTPUT_TARGETS cannot be specified "
            "together with QM_FILES_OUTPUT_VARIABLE.")
    endif()
    if(NOT DEFINED arg_RESOURCE_PREFIX AND NOT DEFINED arg_QM_FILES_OUTPUT_VARIABLE)
        set(arg_RESOURCE_PREFIX "/i18n")
    endif()

    set(scope_args)
    if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.18")
        set(scope_args DIRECTORY ${PROJECT_SOURCE_DIR})
    endif()

    # Determine the .ts file paths if necessary. This must happen before function deferral.
    if(NOT DEFINED arg_TS_FILES)
        if(NOT DEFINED arg_TS_FILE_DIR)
            set(arg_TS_FILE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
        endif()
        if(NOT DEFINED arg_TS_FILE_BASE)
            set(arg_TS_FILE_BASE "${PROJECT_NAME}")
            string(REPLACE " " "-" arg_TS_FILE_BASE "${arg_TS_FILE_BASE}")
        endif()
        set(arg_TS_FILES "")
        foreach(lang IN LISTS QT_I18N_TRANSLATED_LANGUAGES)
            set(ts_file "${arg_TS_FILE_DIR}/${arg_TS_FILE_BASE}_${lang}.ts")
            list(APPEND arg_TS_FILES "${ts_file}")
            set_source_files_properties(${ts_file} ${scope_args} PROPERTIES
                _qt_i18n_translated_language ${lang}
            )
        endforeach()

        # Default the source language to "en" in case the user doesn't use
        # qt_standard_project_setup.
        set(source_lang en)
        if(NOT "${QT_I18N_SOURCE_LANGUAGE}" STREQUAL "")
            set(source_lang ${QT_I18N_SOURCE_LANGUAGE})
        endif()

        # Determine the path to the plurals-only .ts file if necessary.
        if(NOT arg_NO_GENERATE_PLURALS_TS_FILE
            AND NOT DEFINED arg_PLURALS_TS_FILE
            AND NOT "${source_lang}" IN_LIST QT_I18N_TRANSLATED_LANGUAGES)
            set(arg_PLURALS_TS_FILE
                "${arg_TS_FILE_DIR}/${arg_TS_FILE_BASE}_${source_lang}.ts")
            set_source_files_properties(${arg_PLURALS_TS_FILE} ${scope_args} PROPERTIES
                _qt_i18n_translated_language ${source_lang}
            )
        endif()
    endif()

    # Store all .ts file paths in a user-specified variable.
    if(DEFINED arg_TS_FILES_OUTPUT_VARIABLE)
        set("${arg_TS_FILES_OUTPUT_VARIABLE}" ${arg_PLURALS_TS_FILE} ${arg_TS_FILES} PARENT_SCOPE)
    endif()

    # Defer the actual function call if SOURCE_TARGETS was not given and an immediate call was not
    # requested.
    set(source_targets "${arg_SOURCE_TARGETS}")
    if(source_targets STREQUAL "" AND NOT arg_IMMEDIATE_CALL)
        if(DEFINED arg_OUTPUT_TARGETS)
            # We don't have the infrastructure to predict the resource target names.
            message(FATAL_ERROR "Deferring qt6_add_translations is not supported with "
                "OUTPUT_TARGETS. Pass IMMEDIATE_CALL or specify SOURCE_TARGETS.")
        endif()
        if(CMAKE_VERSION VERSION_LESS "3.19")
            message(WARNING
                "qt6_add_translations cannot defer function calls with this CMake version. "
                "Only targets created prior to the qt6_add_translations call are used for i18n. "
                "To avoid this warning, make sure this command is called at the end of the "
                "top-level directory scope and pass the IMMEDIATE_CALL keyword. "
                "Alternatively, upgrade to CMake 3.19 or newer."
            )
            qt6_add_translations(IMMEDIATE_CALL ${ARGV})
        else()
            # Predict the names of generated .qm files.
            if(DEFINED arg_QM_FILES_OUTPUT_VARIABLE)
                set(qm_files "")
                foreach(ts_file IN LISTS arg_TS_FILES arg_PLURALS_TS_FILE)
                    _qt_internal_generated_qm_file_path(qm_file "${ts_file}"
                        "${CMAKE_CURRENT_BINARY_DIR}")
                    list(APPEND qm_files "${qm_file}")
                endforeach()
                set("${arg_QM_FILES_OUTPUT_VARIABLE}" "${qm_files}" PARENT_SCOPE)
            endif()

            # Forward options.
            set(forwarded_args
                IMMEDIATE_CALL
                __QT_INTERNAL_DEFAULT_QM_OUT_DIR "${CMAKE_CURRENT_BINARY_DIR}"
            )
            foreach(keyword IN LISTS options)
                if(arg_${keyword})
                    list(APPEND forwarded_args ${keyword})
                endif()
            endforeach()

            # Forward one-value and multi-value arguments.
            # Filter path variables. We will forward those separately.
            set(to_forward ${oneValueArgs} ${multiValueArgs})
            set(path_variables
                TS_FILES
                PLURALS_TS_FILE
                SOURCES
                INCLUDE_DIRECTORIES
            )
            set(ignored_variables
                TS_FILE_BASE
                TS_FILE_DIR
                TS_FILES_OUTPUT_VARIABLE
            )
            list(REMOVE_ITEM to_forward ${path_variables} ${ignored_variables})
            foreach(keyword IN LISTS to_forward)
                if(DEFINED arg_${keyword})
                    list(APPEND forwarded_args ${keyword} ${arg_${keyword}})
                endif()
            endforeach()

            # Make variables that specify paths absolute.
            # Relative paths are considered to be relative to the current source dir.
            foreach(var IN LISTS path_variables)
                qt_internal_make_paths_absolute(absolute_paths ${arg_${var}})
                if(NOT "${absolute_paths}" STREQUAL "")
                    list(APPEND forwarded_args ${var} ${absolute_paths})
                endif()
            endforeach()

            # The project-level CMakeLists.txt might miss a call to find_package(Qt6 COMPONENTS Core
            # LinguistTools). Then, subsequent function calls like qt_add_resources will fail. To
            # remedy this, pull the packages in.
            cmake_language(EVAL CODE
                "cmake_language(DEFER
                    DIRECTORY \"${PROJECT_SOURCE_DIR}\"
                    CALL find_package Qt6 COMPONENTS Core LinguistTools)")

            # Schedule this command to be called at the end of the project's source dir.
            cmake_language(EVAL CODE
                "cmake_language(DEFER
                    DIRECTORY \"${PROJECT_SOURCE_DIR}\"
                    CALL qt6_add_translations ${forwarded_args})")
        endif()
        return()
    endif()

    if(source_targets STREQUAL "")
        qt6_collect_translation_source_targets(source_targets)
    endif()
    qt6_add_lupdate(
        SOURCE_TARGETS "${source_targets}"
        LUPDATE_TARGET "${arg_LUPDATE_TARGET}"
        TS_FILES "${arg_TS_FILES}"
        PLURALS_TS_FILE "${arg_PLURALS_TS_FILE}"
        SOURCES "${arg_SOURCES}"
        INCLUDE_DIRECTORIES "${arg_INCLUDE_DIRECTORIES}"
        OPTIONS "${arg_LUPDATE_OPTIONS}"
    )
    qt6_add_lrelease(
        LRELEASE_TARGET "${arg_LRELEASE_TARGET}"
        TS_FILES "${arg_TS_FILES}" ${arg_PLURALS_TS_FILE}
        QM_FILES_OUTPUT_VARIABLE qm_files
        OPTIONS "${arg_LRELEASE_OPTIONS}"
        __QT_INTERNAL_DEFAULT_QM_OUT_DIR "${arg___QT_INTERNAL_DEFAULT_QM_OUT_DIR}"
    )

    # Make the .ts files visible in IDEs.
    set(all_ts_files ${arg_TS_FILES} ${arg_PLURALS_TS_FILE})
    foreach(target IN LISTS targets)
        foreach(ts_file IN LISTS all_ts_files)
            _qt_internal_expose_source_file_to_ide(${target} ${ts_file})
        endforeach()
    endforeach()

    if("${QT_I18N_TRANSLATED_LANGUAGES}" STREQUAL "")
        _qt_internal_store_languages_from_ts_files_in_targets("${targets}" "${arg_TS_FILES}")
    endif()

    # Mark .qm files as GENERATED, so that calling _qt_internal_expose_deferred_files_to_ide
    # doesn't cause an error at generation time saying "Cannot find source file:" when
    # qt6_add_lrelease is called from a subdirectory different than the target.
    # The issue happend when the user project called cmake_minimum_required(VERSION)
    # with a version less than 3.20 or set the CMP0118 policy value to OLD.
    set(scope_args "")
    if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.18")
        set(scope_args TARGET_DIRECTORY ${targets})
    endif()
    set_source_files_properties(${qm_files}
        ${scope_args}
        PROPERTIES GENERATED TRUE
    )

    if(NOT "${arg_RESOURCE_PREFIX}" STREQUAL "")
        set(accumulated_out_targets "")
        foreach(target IN LISTS targets)
            get_target_property(target_binary_dir ${target} BINARY_DIR)
            qt6_add_resources(${target} "${target}_translations"
                PREFIX "${arg_RESOURCE_PREFIX}"
                BASE "${target_binary_dir}"
                OUTPUT_TARGETS out_targets
                FILES ${qm_files})
            list(APPEND accumulated_out_targets ${out_targets})
        endforeach()
        if(DEFINED arg_OUTPUT_TARGETS)
            set("${arg_OUTPUT_TARGETS}" "${accumulated_out_targets}" PARENT_SCOPE)
        endif()
    endif()
    if(NOT "${arg_QM_FILES_OUTPUT_VARIABLE}" STREQUAL "")
        set("${arg_QM_FILES_OUTPUT_VARIABLE}" "${qm_files}" PARENT_SCOPE)
    endif()
endfunction()

if(NOT QT_NO_CREATE_VERSIONLESS_FUNCTIONS)
    function(qt_create_translation _qm_files)
        if(QT_DEFAULT_MAJOR_VERSION EQUAL 5)
            qt5_create_translation("${_qm_files}" ${ARGN})
        elseif(QT_DEFAULT_MAJOR_VERSION EQUAL 6)
            qt6_create_translation("${_qm_files}" ${ARGN})
        endif()
        set("${_qm_files}" "${${_qm_files}}" PARENT_SCOPE)
    endfunction()
    function(qt_add_translation _qm_files)
        if(QT_DEFAULT_MAJOR_VERSION EQUAL 5)
            qt5_add_translation("${_qm_files}" ${ARGN})
        elseif(QT_DEFAULT_MAJOR_VERSION EQUAL 6)
            qt6_add_translation("${_qm_files}" ${ARGN})
        endif()
        set("${_qm_files}" "${${_qm_files}}" PARENT_SCOPE)
    endfunction()
    function(qt_collect_translation_source_targets out_var)
        if(QT_DEFAULT_MAJOR_VERSION EQUAL 6)
            qt6_collect_translation_source_targets("${out_var}" ${ARGN})
        else()
            message(FATAL_ERROR "qt_collect_translation_source_targets() is only available in Qt 6.")
        endif()
        set("${out_var}" "${${out_var}}" PARENT_SCOPE)
    endfunction()
    function(qt_add_lupdate)
        if(QT_DEFAULT_MAJOR_VERSION EQUAL 6)
            qt6_add_lupdate(${ARGN})
        else()
            message(FATAL_ERROR "qt_add_lupdate() is only available in Qt 6.")
        endif()
    endfunction()
    function(qt_add_lrelease)
        if(QT_DEFAULT_MAJOR_VERSION EQUAL 6)
            qt6_add_lrelease(${ARGN})
            cmake_parse_arguments(PARSE_ARGV 1 arg "" "QM_FILES_OUTPUT_VARIABLE" "")
            if(arg_QM_FILES_OUTPUT_VARIABLE)
                set(${arg_QM_FILES_OUTPUT_VARIABLE} ${${arg_QM_FILES_OUTPUT_VARIABLE}} PARENT_SCOPE)
            endif()
        else()
            message(FATAL_ERROR "qt_add_lrelease() is only available in Qt 6.")
        endif()
    endfunction()
    function(qt_add_translations)
        if(QT_DEFAULT_MAJOR_VERSION EQUAL 6)
            qt6_add_translations(${ARGN})

            set(no_value_options "")
            set(single_value_options
                TS_FILES_OUTPUT_VARIABLE
                QM_FILES_OUTPUT_VARIABLE
            )
            set(multi_value_options
                OUTPUT_TARGETS
            )
            cmake_parse_arguments(PARSE_ARGV 0 arg
                "${no_value_options}" "${single_value_options}" "${multi_value_options}"
            )

            if(arg_OUTPUT_TARGETS)
                set(${arg_OUTPUT_TARGETS} ${${arg_OUTPUT_TARGETS}} PARENT_SCOPE)
            endif()
            if(arg_TS_FILES_OUTPUT_VARIABLE)
                set(${arg_TS_FILES_OUTPUT_VARIABLE} ${${arg_TS_FILES_OUTPUT_VARIABLE}} PARENT_SCOPE)
            endif()
            if(arg_QM_FILES_OUTPUT_VARIABLE)
                set(${arg_QM_FILES_OUTPUT_VARIABLE} ${${arg_QM_FILES_OUTPUT_VARIABLE}} PARENT_SCOPE)
            endif()
        else()
            message(FATAL_ERROR "qt_add_translations() is only available in Qt 6.")
        endif()
    endfunction()
endif()
