# - cotire (compile time reducer)
#
# See the cotire manual for usage hints.
#
#=============================================================================
# Copyright 2012-2014 Sascha Kratky
#
# Permission is hereby granted, free of charge, to any person
# obtaining a copy of this software and associated documentation
# files (the "Software"), to deal in the Software without
# restriction, including without limitation the rights to use,
# copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following
# conditions:
#
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
# OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
# HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
# WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
# OTHER DEALINGS IN THE SOFTWARE.
#=============================================================================

if(__COTIRE_INCLUDED)
	return()
endif()
set(__COTIRE_INCLUDED TRUE)

# call cmake_minimum_required, but prevent modification of the CMake policy stack in include mode
# cmake_minimum_required also sets the policy version as a side effect, which we have to avoid
if (NOT CMAKE_SCRIPT_MODE_FILE)
	cmake_policy(PUSH)
endif()
# we need the CMake variables CMAKE_SCRIPT_MODE_FILE and CMAKE_ARGV available since 2.8.5
# we need APPEND_STRING option for set_property available since 2.8.6
cmake_minimum_required(VERSION 2.8.6)
if (NOT CMAKE_SCRIPT_MODE_FILE)
	cmake_policy(POP)
endif()

set (COTIRE_CMAKE_MODULE_FILE "${CMAKE_CURRENT_LIST_FILE}")
set (COTIRE_CMAKE_MODULE_VERSION "1.6.1")

include(CMakeParseArguments)
include(ProcessorCount)

function (cotire_determine_compiler_version _language _versionPrefix)
	if (NOT ${_versionPrefix}_VERSION)
		# use CMake's predefined compiler version variable (available since CMake 2.8.8)
		if (DEFINED CMAKE_${_language}_COMPILER_VERSION)
			set (${_versionPrefix}_VERSION "${CMAKE_${_language}_COMPILER_VERSION}")
		elseif (WIN32)
			# cl.exe messes with the output streams unless the environment variable VS_UNICODE_OUTPUT is cleared
			unset (ENV{VS_UNICODE_OUTPUT})
			string (STRIP "${CMAKE_${_language}_COMPILER_ARG1}" _compilerArg1)
			execute_process (COMMAND ${CMAKE_${_language}_COMPILER} ${_compilerArg1}
				ERROR_VARIABLE _versionLine OUTPUT_QUIET TIMEOUT 10)
			string (REGEX REPLACE ".*Version *([0-9]+(\\.[0-9]+)*).*" "\\1" ${_versionPrefix}_VERSION "${_versionLine}")
		else()
			# assume GCC like command line interface
			string (STRIP "${CMAKE_${_language}_COMPILER_ARG1}" _compilerArg1)
			execute_process (COMMAND ${CMAKE_${_language}_COMPILER} ${_compilerArg1} "-dumpversion"
				OUTPUT_VARIABLE ${_versionPrefix}_VERSION
				RESULT_VARIABLE _result
				OUTPUT_STRIP_TRAILING_WHITESPACE TIMEOUT 10)
			if (_result)
				set (${_versionPrefix}_VERSION "")
			endif()
		endif()
		if (${_versionPrefix}_VERSION)
			set (${_versionPrefix}_VERSION "${${_versionPrefix}_VERSION}" CACHE INTERNAL "${_language} compiler version")
		endif()
		set (${_versionPrefix}_VERSION "${${_versionPrefix}_VERSION}" PARENT_SCOPE)
		if (COTIRE_DEBUG)
			message (STATUS "${CMAKE_${_language}_COMPILER} version ${${_versionPrefix}_VERSION}")
		endif()
	endif()
endfunction()

function (cotire_get_source_file_extension _sourceFile _extVar)
	# get_filename_component returns extension from first occurrence of . in file name
	# this function computes the extension from last occurrence of . in file name
	string (FIND "${_sourceFile}" "." _index REVERSE)
	if (_index GREATER -1)
		math (EXPR _index "${_index} + 1")
		string (SUBSTRING "${_sourceFile}" ${_index} -1 _sourceExt)
	else()
		set (_sourceExt "")
	endif()
	set (${_extVar} "${_sourceExt}" PARENT_SCOPE)
endfunction()

macro (cotire_check_is_path_relative_to _path _isRelativeVar)
	set (${_isRelativeVar} FALSE)
	if (IS_ABSOLUTE "${_path}")
		foreach (_dir ${ARGN})
			file (RELATIVE_PATH _relPath "${_dir}" "${_path}")
			if (NOT _relPath OR (NOT IS_ABSOLUTE "${_relPath}" AND NOT "${_relPath}" MATCHES "^\\.\\."))
				set (${_isRelativeVar} TRUE)
				break()
			endif()
		endforeach()
	endif()
endmacro()

function (cotire_filter_language_source_files _language _sourceFilesVar _excludedSourceFilesVar _cotiredSourceFilesVar)
	set (_sourceFiles "")
	set (_excludedSourceFiles "")
	set (_cotiredSourceFiles "")
	if (CMAKE_${_language}_SOURCE_FILE_EXTENSIONS)
		set (_languageExtensions "${CMAKE_${_language}_SOURCE_FILE_EXTENSIONS}")
	else()
		set (_languageExtensions "")
	endif()
	if (CMAKE_${_language}_IGNORE_EXTENSIONS)
		set (_ignoreExtensions "${CMAKE_${_language}_IGNORE_EXTENSIONS}")
	else()
		set (_ignoreExtensions "")
	endif()
	if (COTIRE_UNITY_SOURCE_EXCLUDE_EXTENSIONS)
		set (_excludeExtensions "${COTIRE_UNITY_SOURCE_EXCLUDE_EXTENSIONS}")
	else()
		set (_excludeExtensions "")
	endif()
	if (COTIRE_DEBUG)
		message (STATUS "${_language} source file extensions: ${_languageExtensions}")
		message (STATUS "${_language} ignore extensions: ${_ignoreExtensions}")
		message (STATUS "${_language} exclude extensions: ${_excludeExtensions}")
	endif()
	foreach (_sourceFile ${ARGN})
		get_source_file_property(_sourceIsHeaderOnly "${_sourceFile}" HEADER_FILE_ONLY)
		get_source_file_property(_sourceIsExternal "${_sourceFile}" EXTERNAL_OBJECT)
		get_source_file_property(_sourceIsSymbolic "${_sourceFile}" SYMBOLIC)
		get_source_file_property(_sourceLanguage "${_sourceFile}" LANGUAGE)
		set (_sourceIsFiltered FALSE)
		if (NOT _sourceIsHeaderOnly AND NOT _sourceIsExternal AND NOT _sourceIsSymbolic)
			cotire_get_source_file_extension("${_sourceFile}" _sourceExt)
			if (_sourceExt)
				list (FIND _ignoreExtensions "${_sourceExt}" _ignoreIndex)
				if (_ignoreIndex LESS 0)
					list (FIND _excludeExtensions "${_sourceExt}" _excludeIndex)
					if (_excludeIndex GREATER -1)
						list (APPEND _excludedSourceFiles "${_sourceFile}")
					else()
						list (FIND _languageExtensions "${_sourceExt}" _sourceIndex)
						if (_sourceIndex GREATER -1)
							set (_sourceIsFiltered TRUE)
						elseif ("${_sourceLanguage}" STREQUAL "${_language}")
							# add to excluded sources, if file is not ignored and has correct language without having the correct extension
							list (APPEND _excludedSourceFiles "${_sourceFile}")
						endif()
					endif()
				endif()
			endif()
		endif()
		if (COTIRE_DEBUG)
			message (STATUS "${_sourceFile} filtered=${_sourceIsFiltered} language=${_sourceLanguage} header=${_sourceIsHeaderOnly}")
		endif()
		if (_sourceIsFiltered)
			get_source_file_property(_sourceIsExcluded "${_sourceFile}" COTIRE_EXCLUDED)
			get_source_file_property(_sourceIsCotired "${_sourceFile}" COTIRE_TARGET)
			get_source_file_property(_sourceCompileFlags "${_sourceFile}" COMPILE_FLAGS)
			if (COTIRE_DEBUG)
				message (STATUS "${_sourceFile} excluded=${_sourceIsExcluded} cotired=${_sourceIsCotired} compileFlags=${_sourceCompileFlags}")
			endif()
			if (_sourceIsCotired)
				list (APPEND _cotiredSourceFiles "${_sourceFile}")
			elseif (_sourceIsExcluded OR _sourceCompileFlags)
				list (APPEND _excludedSourceFiles "${_sourceFile}")
			else()
				list (APPEND _sourceFiles "${_sourceFile}")
			endif()
		endif()
	endforeach()
	if (COTIRE_DEBUG)
		message (STATUS "All: ${ARGN}")
		message (STATUS "${_language}: ${_sourceFiles}")
		message (STATUS "Excluded: ${_excludedSourceFiles}")
		message (STATUS "Cotired: ${_cotiredSourceFiles}")
	endif()
	set (${_sourceFilesVar} ${_sourceFiles} PARENT_SCOPE)
	set (${_excludedSourceFilesVar} ${_excludedSourceFiles} PARENT_SCOPE)
	set (${_cotiredSourceFilesVar} ${_cotiredSourceFiles} PARENT_SCOPE)
endfunction()

function (cotire_get_objects_with_property_on _filteredObjectsVar _property _type)
	set (_filteredObjects "")
	foreach (_object ${ARGN})
		get_property(_isSet ${_type} "${_object}" PROPERTY ${_property} SET)
		if (_isSet)
			get_property(_propertyValue ${_type} "${_object}" PROPERTY ${_property})
			if (_propertyValue)
				list (APPEND _filteredObjects "${_object}")
			endif()
		endif()
	endforeach()
	set (${_filteredObjectsVar} ${_filteredObjects} PARENT_SCOPE)
endfunction()

function (cotire_get_objects_with_property_off _filteredObjectsVar _property _type)
	set (_filteredObjects "")
	foreach (_object ${ARGN})
		get_property(_isSet ${_type} "${_object}" PROPERTY ${_property} SET)
		if (_isSet)
			get_property(_propertyValue ${_type} "${_object}" PROPERTY ${_property})
			if (NOT _propertyValue)
				list (APPEND _filteredObjects "${_object}")
			endif()
		endif()
	endforeach()
	set (${_filteredObjectsVar} ${_filteredObjects} PARENT_SCOPE)
endfunction()

function (cotire_get_source_file_property_values _valuesVar _property)
	set (_values "")
	foreach (_sourceFile ${ARGN})
		get_source_file_property(_propertyValue "${_sourceFile}" ${_property})
		if (_propertyValue)
			list (APPEND _values "${_propertyValue}")
		endif()
	endforeach()
	set (${_valuesVar} ${_values} PARENT_SCOPE)
endfunction()

function (cotire_resolve_config_properites _configurations _propertiesVar)
	set (_properties "")
	foreach (_property ${ARGN})
		if ("${_property}" MATCHES "<CONFIG>")
			foreach (_config ${_configurations})
				string (TOUPPER "${_config}" _upperConfig)
				string (REPLACE "<CONFIG>" "${_upperConfig}" _configProperty "${_property}")
				list (APPEND _properties ${_configProperty})
			endforeach()
		else()
			list (APPEND _properties ${_property})
		endif()
	endforeach()
	set (${_propertiesVar} ${_properties} PARENT_SCOPE)
endfunction()

function (cotire_copy_set_properites _configurations _type _source _target)
	cotire_resolve_config_properites("${_configurations}" _properties ${ARGN})
	foreach (_property ${_properties})
		get_property(_isSet ${_type} ${_source} PROPERTY ${_property} SET)
		if (_isSet)
			get_property(_propertyValue ${_type} ${_source} PROPERTY ${_property})
			set_property(${_type} ${_target} PROPERTY ${_property} "${_propertyValue}")
		endif()
	endforeach()
endfunction()

function (cotire_filter_compile_flags _language _flagFilter _matchedOptionsVar _unmatchedOptionsVar)
	if (WIN32 AND CMAKE_${_language}_COMPILER_ID MATCHES "MSVC|Intel")
		set (_flagPrefix "[/-]")
	else()
		set (_flagPrefix "--?")
	endif()
	set (_optionFlag "")
	set (_matchedOptions "")
	set (_unmatchedOptions "")
	foreach (_compileFlag ${ARGN})
		if (_compileFlag)
			if (_optionFlag AND NOT "${_compileFlag}" MATCHES "^${_flagPrefix}")
				# option with separate argument
				list (APPEND _matchedOptions "${_compileFlag}")
				set (_optionFlag "")
			elseif ("${_compileFlag}" MATCHES "^(${_flagPrefix})(${_flagFilter})$")
				# remember option
				set (_optionFlag "${CMAKE_MATCH_2}")
			elseif ("${_compileFlag}" MATCHES "^(${_flagPrefix})(${_flagFilter})(.+)$")
				# option with joined argument
				list (APPEND _matchedOptions "${CMAKE_MATCH_3}")
				set (_optionFlag "")
			else()
				# flush remembered option
				if (_optionFlag)
					list (APPEND _matchedOptions "${_optionFlag}")
					set (_optionFlag "")
				endif()
				# add to unfiltered options
				list (APPEND _unmatchedOptions "${_compileFlag}")
			endif()
		endif()
	endforeach()
	if (_optionFlag)
		list (APPEND _matchedOptions "${_optionFlag}")
	endif()
	if (COTIRE_DEBUG)
		message (STATUS "Filter ${_flagFilter}")
		if (_matchedOptions)
			message (STATUS "Matched ${_matchedOptions}")
		endif()
		if (_unmatchedOptions)
			message (STATUS "Unmatched ${_unmatchedOptions}")
		endif()
	endif()
	set (${_matchedOptionsVar} ${_matchedOptions} PARENT_SCOPE)
	set (${_unmatchedOptionsVar} ${_unmatchedOptions} PARENT_SCOPE)
endfunction()

function (cotire_get_target_compile_flags _config _language _directory _target _flagsVar)
	string (TOUPPER "${_config}" _upperConfig)
	# collect options from CMake language variables
	set (_compileFlags "")
	if (CMAKE_${_language}_FLAGS)
		set (_compileFlags "${_compileFlags} ${CMAKE_${_language}_FLAGS}")
	endif()
	if (CMAKE_${_language}_FLAGS_${_upperConfig})
		set (_compileFlags "${_compileFlags} ${CMAKE_${_language}_FLAGS_${_upperConfig}}")
	endif()
	if (_target)
		# add option from CMake target type variable
		get_target_property(_targetType ${_target} TYPE)
		if (POLICY CMP0018)
			# handle POSITION_INDEPENDENT_CODE property introduced with CMake 2.8.9 if policy CMP0018 is turned on
			cmake_policy(GET CMP0018 _PIC_Policy)
		else()
			# default to old behavior
			set (_PIC_Policy "OLD")
		endif()
		if (COTIRE_DEBUG)
			message(STATUS "CMP0018=${_PIC_Policy}")
		endif()
		if (_PIC_Policy STREQUAL "NEW")
			# NEW behavior: honor the POSITION_INDEPENDENT_CODE target property
			get_target_property(_targetPIC ${_target} POSITION_INDEPENDENT_CODE)
			if (_targetPIC)
				if (_targetType STREQUAL "EXECUTABLE" AND CMAKE_${_language}_COMPILE_OPTIONS_PIE)
					set (_compileFlags "${_compileFlags} ${CMAKE_${_language}_COMPILE_OPTIONS_PIE}")
				elseif (CMAKE_${_language}_COMPILE_OPTIONS_PIC)
					set (_compileFlags "${_compileFlags} ${CMAKE_${_language}_COMPILE_OPTIONS_PIC}")
				endif()
			endif()
		else()
			# OLD behavior or policy not set: use the value of CMAKE_SHARED_LIBRARY_<Lang>_FLAGS
			if (_targetType STREQUAL "MODULE_LIBRARY")
				# flags variable for module library uses different name SHARED_MODULE
				# (e.g., CMAKE_SHARED_MODULE_C_FLAGS)
				set (_targetType SHARED_MODULE)
			endif()
			if (CMAKE_${_targetType}_${_language}_FLAGS)
				set (_compileFlags "${_compileFlags} ${CMAKE_${_targetType}_${_language}_FLAGS}")
			endif()
		endif()
	endif()
	if (_directory)
		# add_definitions may have been used to add flags to the compiler command
		get_directory_property(_dirDefinitions DIRECTORY "${_directory}" DEFINITIONS)
		if (_dirDefinitions)
			set (_compileFlags "${_compileFlags} ${_dirDefinitions}")
		endif()
	endif()
	if (_target)
		# add target compile options
		get_target_property(_targetflags ${_target} COMPILE_FLAGS)
		if (_targetflags)
			set (_compileFlags "${_compileFlags} ${_targetflags}")
		endif()
		get_target_property(_targetOptions ${_target} COMPILE_OPTIONS)
		if (_targetOptions)
			set (_compileFlags "${_compileFlags} ${_targetOptions}")
		endif()
	endif()
	if (UNIX)
		separate_arguments(_compileFlags UNIX_COMMAND "${_compileFlags}")
	elseif(WIN32)
		separate_arguments(_compileFlags WINDOWS_COMMAND "${_compileFlags}")
	else()
		separate_arguments(_compileFlags)
	endif()
	# platform specific flags
	if (APPLE)
		get_target_property(_architectures ${_target} OSX_ARCHITECTURES_${_upperConfig})
		if (NOT _architectures)
			get_target_property(_architectures ${_target} OSX_ARCHITECTURES)
		endif()
		foreach (_arch ${_architectures})
			list (APPEND _compileFlags "-arch" "${_arch}")
		endforeach()
		if (CMAKE_OSX_SYSROOT)
			if (CMAKE_${_language}_SYSROOT_FLAG)
				list (APPEND _compileFlags "${CMAKE_${_language}_SYSROOT_FLAG}" "${CMAKE_OSX_SYSROOT}")
			else()
				list (APPEND _compileFlags "-isysroot" "${CMAKE_OSX_SYSROOT}")
			endif()
		endif()
		if (CMAKE_OSX_DEPLOYMENT_TARGET)
			if (CMAKE_${_language}_OSX_DEPLOYMENT_TARGET_FLAG)
				list (APPEND _compileFlags "${CMAKE_${_language}_OSX_DEPLOYMENT_TARGET_FLAG}${CMAKE_OSX_DEPLOYMENT_TARGET}")
			else()
				list (APPEND _compileFlags "-mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET}")
			endif()
		endif()
	endif()
	if (COTIRE_DEBUG AND _compileFlags)
		message (STATUS "Target ${_target} compile flags ${_compileFlags}")
	endif()
	set (${_flagsVar} ${_compileFlags} PARENT_SCOPE)
endfunction()

function (cotire_get_target_include_directories _config _language _targetSourceDir _targetBinaryDir _target _includeDirsVar)
	set (_includeDirs "")
	# default include dirs
	if (CMAKE_INCLUDE_CURRENT_DIR)
		list (APPEND _includeDirs "${_targetBinaryDir}")
		list (APPEND _includeDirs "${_targetSourceDir}")
	endif()
	# parse additional include directories from target compile flags
	set (_targetFlags "")
	cotire_get_target_compile_flags("${_config}" "${_language}" "${_targetSourceDir}" "${_target}" _targetFlags)
	cotire_filter_compile_flags("${_language}" "I" _dirs _ignore ${_targetFlags})
	if (_dirs)
		list (APPEND _includeDirs ${_dirs})
	endif()
	# target include directories
	get_directory_property(_dirs DIRECTORY "${_targetSourceDir}" INCLUDE_DIRECTORIES)
	if (_target)
		get_target_property(_targetDirs ${_target} INCLUDE_DIRECTORIES)
		if (_targetDirs)
			list (APPEND _dirs ${_targetDirs})
			list (REMOVE_DUPLICATES _dirs)
		endif()
	endif()
	list (LENGTH _includeDirs _projectInsertIndex)
	foreach (_dir ${_dirs})
		if (CMAKE_INCLUDE_DIRECTORIES_PROJECT_BEFORE)
			cotire_check_is_path_relative_to("${_dir}" _isRelative "${CMAKE_SOURCE_DIR}" "${CMAKE_BINARY_DIR}")
			if (_isRelative)
				list (LENGTH _includeDirs _len)
				if (_len EQUAL _projectInsertIndex)
					list (APPEND _includeDirs "${_dir}")
				else()
					list (INSERT _includeDirs _projectInsertIndex "${_dir}")
				endif()
				math (EXPR _projectInsertIndex "${_projectInsertIndex} + 1")
			else()
				list (APPEND _includeDirs "${_dir}")
			endif()
		else()
			list (APPEND _includeDirs "${_dir}")
		endif()
	endforeach()
	list (REMOVE_DUPLICATES _includeDirs)
	if (CMAKE_${_language}_IMPLICIT_INCLUDE_DIRECTORIES)
		list (REMOVE_ITEM _includeDirs ${CMAKE_${_language}_IMPLICIT_INCLUDE_DIRECTORIES})
	endif()
	if (COTIRE_DEBUG AND _includeDirs)
		message (STATUS "Target ${_target} include dirs ${_includeDirs}")
	endif()
	set (${_includeDirsVar} ${_includeDirs} PARENT_SCOPE)
endfunction()

macro (cotire_make_C_identifier _identifierVar _str)
	if (CMAKE_VERSION VERSION_LESS "2.8.12")
		# mimic CMake SystemTools::MakeCindentifier behavior
		if ("${_str}" MATCHES "^[0-9].+$")
			set (_str "_${str}")
		endif()
		string (REGEX REPLACE "[^a-zA-Z0-9]" "_" ${_identifierVar} "${_str}")
	else()
		string (MAKE_C_IDENTIFIER "${_identifierVar}" "${_str}")
	endif()
endmacro()

function (cotire_get_target_export_symbol _target _exportSymbolVar)
	set (_exportSymbol "")
	get_target_property(_targetType ${_target} TYPE)
	get_target_property(_enableExports ${_target} ENABLE_EXPORTS)
	if (_targetType MATCHES "(SHARED|MODULE)_LIBRARY" OR
		(_targetType STREQUAL "EXECUTABLE" AND _enableExports))
		get_target_property(_exportSymbol ${_target} DEFINE_SYMBOL)
		if (NOT _exportSymbol)
			set (_exportSymbol "${_target}_EXPORTS")
		endif()
		cotire_make_C_identifier(_exportSymbol "${_exportSymbol}")
	endif()
	set (${_exportSymbolVar} ${_exportSymbol} PARENT_SCOPE)
endfunction()

function (cotire_get_target_compile_definitions _config _language _directory _target _definitionsVar)
	string (TOUPPER "${_config}" _upperConfig)
	set (_configDefinitions "")
	# CMAKE_INTDIR for multi-configuration build systems
	if (NOT "${CMAKE_CFG_INTDIR}" STREQUAL ".")
		list (APPEND _configDefinitions "CMAKE_INTDIR=\"${_config}\"")
	endif()
	# target export define symbol
	cotire_get_target_export_symbol("${_target}" _defineSymbol)
	if (_defineSymbol)
		list (APPEND _configDefinitions "${_defineSymbol}")
	endif()
	# directory compile definitions
	get_directory_property(_definitions DIRECTORY "${_directory}" COMPILE_DEFINITIONS)
	if (_definitions)
		list (APPEND _configDefinitions ${_definitions})
	endif()
	get_directory_property(_definitions DIRECTORY "${_directory}" COMPILE_DEFINITIONS_${_upperConfig})
	if (_definitions)
		list (APPEND _configDefinitions ${_definitions})
	endif()
	# target compile definitions
	get_target_property(_definitions ${_target} COMPILE_DEFINITIONS)
	if (_definitions)
		list (APPEND _configDefinitions ${_definitions})
	endif()
	get_target_property(_definitions ${_target} COMPILE_DEFINITIONS_${_upperConfig})
	if (_definitions)
		list (APPEND _configDefinitions ${_definitions})
	endif()
	# parse additional compile definitions from target compile flags
	# and don't look at directory compile definitions, which we already handled
	set (_targetFlags "")
	cotire_get_target_compile_flags("${_config}" "${_language}" "" "${_target}" _targetFlags)
	cotire_filter_compile_flags("${_language}" "D" _definitions _ignore ${_targetFlags})
	if (_definitions)
		list (APPEND _configDefinitions ${_definitions})
	endif()
	list (REMOVE_DUPLICATES _configDefinitions)
	if (COTIRE_DEBUG AND _configDefinitions)
		message (STATUS "Target ${_target} compile definitions ${_configDefinitions}")
	endif()
	set (${_definitionsVar} ${_configDefinitions} PARENT_SCOPE)
endfunction()

function (cotire_get_target_compiler_flags _config _language _directory _target _compilerFlagsVar)
	# parse target compile flags omitting compile definitions and include directives
	set (_targetFlags "")
	cotire_get_target_compile_flags("${_config}" "${_language}" "${_directory}" "${_target}" _targetFlags)
	set (_compilerFlags "")
	cotire_filter_compile_flags("${_language}" "[ID]" _ignore _compilerFlags ${_targetFlags})
	if (COTIRE_DEBUG AND _compilerFlags)
		message (STATUS "Target ${_target} compiler flags ${_compilerFlags}")
	endif()
	set (${_compilerFlagsVar} ${_compilerFlags} PARENT_SCOPE)
endfunction()

function (cotire_add_sys_root_paths _pathsVar)
	if (APPLE)
		if (CMAKE_OSX_SYSROOT AND CMAKE_${_language}_HAS_ISYSROOT)
			foreach (_path IN LISTS ${_pathsVar})
				if (IS_ABSOLUTE "${_path}")
					get_filename_component(_path "${CMAKE_OSX_SYSROOT}/${_path}" ABSOLUTE)
					if (EXISTS "${_path}")
						list (APPEND ${_pathsVar} "${_path}")
					endif()
				endif()
			endforeach()
		endif()
	endif()
	set (${_pathsVar} ${${_pathsVar}} PARENT_SCOPE)
	if (COTIRE_DEBUG)
		message (STATUS "${_pathsVar}=${${_pathsVar}}")
	endif()
endfunction()

function (cotire_get_source_extra_properties _sourceFile _pattern _resultVar)
	set (_extraProperties ${ARGN})
	set (_result "")
	if (_extraProperties)
		list (FIND _extraProperties "${_sourceFile}" _index)
		if (_index GREATER -1)
			math (EXPR _index "${_index} + 1")
			list (LENGTH _extraProperties _len)
			math (EXPR _len "${_len} - 1")
			foreach (_index RANGE ${_index} ${_len})
				list (GET _extraProperties ${_index} _value)
				if (_value MATCHES "${_pattern}")
					list (APPEND _result "${_value}")
				else()
					break()
				endif()
			endforeach()
		endif()
	endif()
	set (${_resultVar} ${_result} PARENT_SCOPE)
endfunction()

function (cotire_get_source_compile_definitions _config _language _sourceFile _definitionsVar)
	set (_compileDefinitions "")
	if (NOT CMAKE_SCRIPT_MODE_FILE)
		string (TOUPPER "${_config}" _upperConfig)
		get_source_file_property(_definitions "${_sourceFile}" COMPILE_DEFINITIONS)
		if (_definitions)
			list (APPEND _compileDefinitions ${_definitions})
		endif()
		get_source_file_property(_definitions "${_sourceFile}" COMPILE_DEFINITIONS_${_upperConfig})
		if (_definitions)
			list (APPEND _compileDefinitions ${_definitions})
		endif()
	endif()
	cotire_get_source_extra_properties("${_sourceFile}" "^[a-zA-Z0-9_]+(=.*)?$" _definitions ${ARGN})
	if (_definitions)
		list (APPEND _compileDefinitions ${_definitions})
	endif()
	if (COTIRE_DEBUG AND _compileDefinitions)
		message (STATUS "Source ${_sourceFile} compile definitions ${_compileDefinitions}")
	endif()
	set (${_definitionsVar} ${_compileDefinitions} PARENT_SCOPE)
endfunction()

function (cotire_get_source_files_compile_definitions _config _language _definitionsVar)
	set (_configDefinitions "")
	foreach (_sourceFile ${ARGN})
		cotire_get_source_compile_definitions("${_config}" "${_language}" "${_sourceFile}" _sourceDefinitions)
		if (_sourceDefinitions)
			list (APPEND _configDefinitions "${_sourceFile}" ${_sourceDefinitions} "-")
		endif()
	endforeach()
	set (${_definitionsVar} ${_configDefinitions} PARENT_SCOPE)
endfunction()

function (cotire_get_source_undefs _sourceFile _property _sourceUndefsVar)
	set (_sourceUndefs "")
	if (NOT CMAKE_SCRIPT_MODE_FILE)
		get_source_file_property(_undefs "${_sourceFile}" ${_property})
		if (_undefs)
			list (APPEND _sourceUndefs ${_undefs})
		endif()
	endif()
	cotire_get_source_extra_properties("${_sourceFile}" "^[a-zA-Z0-9_]+$" _undefs ${ARGN})
	if (_undefs)
		list (APPEND _sourceUndefs ${_undefs})
	endif()
	if (COTIRE_DEBUG AND _sourceUndefs)
		message (STATUS "Source ${_sourceFile} ${_property} undefs ${_sourceUndefs}")
	endif()
	set (${_sourceUndefsVar} ${_sourceUndefs} PARENT_SCOPE)
endfunction()

function (cotire_get_source_files_undefs _property _sourceUndefsVar)
	set (_sourceUndefs "")
	foreach (_sourceFile ${ARGN})
		cotire_get_source_undefs("${_sourceFile}" ${_property} _undefs)
		if (_undefs)
			list (APPEND _sourceUndefs "${_sourceFile}" ${_undefs} "-")
		endif()
	endforeach()
	set (${_sourceUndefsVar} ${_sourceUndefs} PARENT_SCOPE)
endfunction()

macro (cotire_set_cmd_to_prologue _cmdVar)
	set (${_cmdVar} "${CMAKE_COMMAND}")
	if (COTIRE_DEBUG)
		list (APPEND ${_cmdVar} "--warn-uninitialized")
	endif()
	list (APPEND ${_cmdVar} "-DCOTIRE_BUILD_TYPE:STRING=$<CONFIGURATION>")
	if (COTIRE_VERBOSE)
		list (APPEND ${_cmdVar} "-DCOTIRE_VERBOSE:BOOL=ON")
	elseif("${CMAKE_GENERATOR}" MATCHES "Makefiles")
		list (APPEND ${_cmdVar} "-DCOTIRE_VERBOSE:BOOL=$(VERBOSE)")
	endif()
endmacro()

function (cotire_init_compile_cmd _cmdVar _language _compilerExe _compilerArg1)
	if (NOT _compilerExe)
		set (_compilerExe "${CMAKE_${_language}_COMPILER}")
	endif()
	if (NOT _compilerArg1)
		set (_compilerArg1 ${CMAKE_${_language}_COMPILER_ARG1})
	endif()
	string (STRIP "${_compilerArg1}" _compilerArg1)
	set (${_cmdVar} "${_compilerExe}" ${_compilerArg1} PARENT_SCOPE)
endfunction()

macro (cotire_add_definitions_to_cmd _cmdVar _language)
	foreach (_definition ${ARGN})
		if (WIN32 AND CMAKE_${_language}_COMPILER_ID MATCHES "MSVC|Intel")
			list (APPEND ${_cmdVar} "/D${_definition}")
		else()
			list (APPEND ${_cmdVar} "-D${_definition}")
		endif()
	endforeach()
endmacro()

macro (cotire_add_includes_to_cmd _cmdVar _language)
	foreach (_include ${ARGN})
		if (WIN32 AND CMAKE_${_language}_COMPILER_ID MATCHES "MSVC|Intel")
			file (TO_NATIVE_PATH "${_include}" _include)
			list (APPEND ${_cmdVar} "/I${_include}")
		else()
			list (APPEND ${_cmdVar} "-I${_include}")
		endif()
	endforeach()
endmacro()

macro (cotire_add_frameworks_to_cmd _cmdVar _language)
	if (APPLE)
		set (_frameWorkDirs "")
		foreach (_include ${ARGN})
			if (IS_ABSOLUTE "${_include}" AND _include MATCHES "\\.framework$")
				get_filename_component(_frameWorkDir "${_include}" PATH)
				list (APPEND _frameWorkDirs "${_frameWorkDir}")
			endif()
		endforeach()
		if (_frameWorkDirs)
			list (REMOVE_DUPLICATES _frameWorkDirs)
			foreach (_frameWorkDir ${_frameWorkDirs})
				list (APPEND ${_cmdVar} "-F${_frameWorkDir}")
			endforeach()
		endif()
	endif()
endmacro()

macro (cotire_add_compile_flags_to_cmd _cmdVar)
	foreach (_flag ${ARGN})
		list (APPEND ${_cmdVar} "${_flag}")
	endforeach()
endmacro()

function (cotire_check_file_up_to_date _fileIsUpToDateVar _file)
	set (${_fileIsUpToDateVar} FALSE PARENT_SCOPE)
	set (_triggerFile "")
	foreach (_dependencyFile ${ARGN})
		if (EXISTS "${_dependencyFile}" AND "${_dependencyFile}" IS_NEWER_THAN "${_file}")
			set (_triggerFile "${_dependencyFile}")
			break()
		endif()
	endforeach()
	get_filename_component(_fileName "${_file}" NAME)
	if (EXISTS "${_file}")
		if (_triggerFile)
			if (COTIRE_VERBOSE)
				message (STATUS "${_fileName} update triggered by ${_triggerFile} change.")
			endif()
		else()
			if (COTIRE_VERBOSE)
				message (STATUS "${_fileName} is up-to-date.")
			endif()
			set (${_fileIsUpToDateVar} TRUE PARENT_SCOPE)
		endif()
	else()
		if (COTIRE_VERBOSE)
			message (STATUS "${_fileName} does not exist yet.")
		endif()
	endif()
endfunction()

macro (cotire_find_closest_relative_path _headerFile _includeDirs _relPathVar)
	set (${_relPathVar} "")
	foreach (_includeDir ${_includeDirs})
		if (IS_DIRECTORY "${_includeDir}")
			file (RELATIVE_PATH _relPath "${_includeDir}" "${_headerFile}")
			if (NOT IS_ABSOLUTE "${_relPath}" AND NOT "${_relPath}" MATCHES "^\\.\\.")
				string (LENGTH "${${_relPathVar}}" _closestLen)
				string (LENGTH "${_relPath}" _relLen)
				if (_closestLen EQUAL 0 OR _relLen LESS _closestLen)
					set (${_relPathVar} "${_relPath}")
				endif()
			endif()
		elseif ("${_includeDir}" STREQUAL "${_headerFile}")
			# if path matches exactly, return short non-empty string
			set (${_relPathVar} "1")
			break()
		endif()
	endforeach()
endmacro()

macro (cotire_check_header_file_location _headerFile _insideIncudeDirs _outsideIncudeDirs _headerIsInside)
	# check header path against ignored and honored include directories
	cotire_find_closest_relative_path("${_headerFile}" "${_insideIncudeDirs}" _insideRelPath)
	if (_insideRelPath)
		# header is inside, but could be become outside if there is a shorter outside match
		cotire_find_closest_relative_path("${_headerFile}" "${_outsideIncudeDirs}" _outsideRelPath)
		if (_outsideRelPath)
			string (LENGTH "${_insideRelPath}" _insideRelPathLen)
			string (LENGTH "${_outsideRelPath}" _outsideRelPathLen)
			if (_outsideRelPathLen LESS _insideRelPathLen)
				set (${_headerIsInside} FALSE)
			else()
				set (${_headerIsInside} TRUE)
			endif()
		else()
			set (${_headerIsInside} TRUE)
		endif()
	else()
		# header is outside
		set (${_headerIsInside} FALSE)
	endif()
endmacro()

macro (cotire_check_ignore_header_file_path _headerFile _headerIsIgnoredVar)
	if (NOT EXISTS "${_headerFile}")
		set (${_headerIsIgnoredVar} TRUE)
	elseif (IS_DIRECTORY "${_headerFile}")
		set (${_headerIsIgnoredVar} TRUE)
	elseif ("${_headerFile}" MATCHES "\\.\\.|[_-]fixed" AND "${_headerFile}" MATCHES "\\.h$")
		# heuristic: ignore C headers with embedded parent directory references or "-fixed" or "_fixed" in path
		# these often stem from using GCC #include_next tricks, which may break the precompiled header compilation
		# with the error message "error: no include path in which to search for header.h"
		set (${_headerIsIgnoredVar} TRUE)
	else()
		set (${_headerIsIgnoredVar} FALSE)
	endif()
endmacro()

macro (cotire_check_ignore_header_file_ext _headerFile _ignoreExtensionsVar _headerIsIgnoredVar)
	# check header file extension
	cotire_get_source_file_extension("${_headerFile}" _headerFileExt)
	set (${_headerIsIgnoredVar} FALSE)
	if (_headerFileExt)
		list (FIND ${_ignoreExtensionsVar} "${_headerFileExt}" _index)
		if (_index GREATER -1)
			set (${_headerIsIgnoredVar} TRUE)
		endif()
	endif()
endmacro()

macro (cotire_parse_line _line _headerFileVar _headerDepthVar)
	if (MSVC)
		# cl.exe /showIncludes output looks different depending on the language pack used, e.g.:
		# English: "Note: including file:   C:\directory\file"
		# German: "Hinweis: Einlesen der Datei:   C:\directory\file"
		# We use a very general regular expression, relying on the presence of the : characters
		if (_line MATCHES ":( +)([^:]+:[^:]+)$")
			# Visual Studio compiler output
			string (LENGTH "${CMAKE_MATCH_1}" ${_headerDepthVar})
			get_filename_component(${_headerFileVar} "${CMAKE_MATCH_2}" ABSOLUTE)
		else()
			set (${_headerFileVar} "")
			set (${_headerDepthVar} 0)
		endif()
	else()
		if (_line MATCHES "^(\\.+) (.*)$")
			# GCC like output
			string (LENGTH "${CMAKE_MATCH_1}" ${_headerDepthVar})
			if (IS_ABSOLUTE "${CMAKE_MATCH_2}")
				set (${_headerFileVar} "${CMAKE_MATCH_2}")
			else()
				get_filename_component(${_headerFileVar} "${CMAKE_MATCH_2}" REALPATH)
			endif()
		else()
			set (${_headerFileVar} "")
			set (${_headerDepthVar} 0)
		endif()
	endif()
endmacro()

function (cotire_parse_includes _language _scanOutput _ignoredIncudeDirs _honoredIncudeDirs _ignoredExtensions _selectedIncludesVar _unparsedLinesVar)
	if (WIN32)
		# prevent CMake macro invocation errors due to backslash characters in Windows paths
		string (REPLACE "\\" "/" _scanOutput "${_scanOutput}")
	endif()
	# canonize slashes
	string (REPLACE "//" "/" _scanOutput "${_scanOutput}")
	# prevent semicolon from being interpreted as a line separator
	string (REPLACE ";" "\\;" _scanOutput "${_scanOutput}")
	# then separate lines
	string (REGEX REPLACE "\n" ";" _scanOutput "${_scanOutput}")
	list (LENGTH _scanOutput _len)
	# remove duplicate lines to speed up parsing
	list (REMOVE_DUPLICATES _scanOutput)
	list (LENGTH _scanOutput _uniqueLen)
	if (COTIRE_VERBOSE OR COTIRE_DEBUG)
		message (STATUS "Scanning ${_uniqueLen} unique lines of ${_len} for includes")
		if (_ignoredExtensions)
			message (STATUS "Ignored extensions: ${_ignoredExtensions}")
		endif()
		if (_ignoredIncudeDirs)
			message (STATUS "Ignored paths: ${_ignoredIncudeDirs}")
		endif()
		if (_honoredIncudeDirs)
			message (STATUS "Included paths: ${_honoredIncudeDirs}")
		endif()
	endif()
	set (_sourceFiles ${ARGN})
	set (_selectedIncludes "")
	set (_unparsedLines "")
	# stack keeps track of inside/outside project status of processed header files
	set (_headerIsInsideStack "")
	foreach (_line IN LISTS _scanOutput)
		if (_line)
			cotire_parse_line("${_line}" _headerFile _headerDepth)
			if (_headerFile)
				cotire_check_header_file_location("${_headerFile}" "${_ignoredIncudeDirs}" "${_honoredIncudeDirs}" _headerIsInside)
				if (COTIRE_DEBUG)
					message (STATUS "${_headerDepth}: ${_headerFile} ${_headerIsInside}")
				endif()
				# update stack
				list (LENGTH _headerIsInsideStack _stackLen)
				if (_headerDepth GREATER _stackLen)
					math (EXPR _stackLen "${_stackLen} + 1")
					foreach (_index RANGE ${_stackLen} ${_headerDepth})
						list (APPEND _headerIsInsideStack ${_headerIsInside})
					endforeach()
				else()
					foreach (_index RANGE ${_headerDepth} ${_stackLen})
						list (REMOVE_AT _headerIsInsideStack -1)
					endforeach()
					list (APPEND _headerIsInsideStack ${_headerIsInside})
				endif()
				if (COTIRE_DEBUG)
					message (STATUS "${_headerIsInsideStack}")
				endif()
				# header is a candidate if it is outside project
				if (NOT _headerIsInside)
					# get parent header file's inside/outside status
					if (_headerDepth GREATER 1)
						math (EXPR _index "${_headerDepth} - 2")
						list (GET _headerIsInsideStack ${_index} _parentHeaderIsInside)
					else()
						set (_parentHeaderIsInside TRUE)
					endif()
					# select header file if parent header file is inside project
					# (e.g., a project header file that includes a standard header file)
					if (_parentHeaderIsInside)
						cotire_check_ignore_header_file_path("${_headerFile}" _headerIsIgnored)
						if (NOT _headerIsIgnored)
							cotire_check_ignore_header_file_ext("${_headerFile}" _ignoredExtensions _headerIsIgnored)
							if (NOT _headerIsIgnored)
								list (APPEND _selectedIncludes "${_headerFile}")
							else()
								# fix header's inside status on stack, it is ignored by extension now
								list (REMOVE_AT _headerIsInsideStack -1)
								list (APPEND _headerIsInsideStack TRUE)
							endif()
						endif()
						if (COTIRE_DEBUG)
							message (STATUS "${_headerFile} ${_ignoredExtensions} ${_headerIsIgnored}")
						endif()
					endif()
				endif()
			else()
				if (MSVC)
					# for cl.exe do not keep unparsed lines which solely consist of a source file name
					string (FIND "${_sourceFiles}" "${_line}" _index)
					if (_index LESS 0)
						list (APPEND _unparsedLines "${_line}")
					endif()
				else()
					list (APPEND _unparsedLines "${_line}")
				endif()
			endif()
		endif()
	endforeach()
	list (REMOVE_DUPLICATES _selectedIncludes)
	set (${_selectedIncludesVar} ${_selectedIncludes} PARENT_SCOPE)
	set (${_unparsedLinesVar} ${_unparsedLines} PARENT_SCOPE)
endfunction()

function (cotire_scan_includes _includesVar)
	set(_options "")
	set(_oneValueArgs COMPILER_ID COMPILER_EXECUTABLE COMPILER_VERSION LANGUAGE UNPARSED_LINES)
	set(_multiValueArgs COMPILE_DEFINITIONS COMPILE_FLAGS INCLUDE_DIRECTORIES IGNORE_PATH INCLUDE_PATH IGNORE_EXTENSIONS)
	cmake_parse_arguments(_option "${_options}" "${_oneValueArgs}" "${_multiValueArgs}" ${ARGN})
	set (_sourceFiles ${_option_UNPARSED_ARGUMENTS})
	if (NOT _option_LANGUAGE)
		set (_option_LANGUAGE "CXX")
	endif()
	if (NOT _option_COMPILER_ID)
		set (_option_COMPILER_ID "${CMAKE_${_option_LANGUAGE}_ID}")
	endif()
	set (_cmd "${_option_COMPILER_EXECUTABLE}" ${_option_COMPILER_ARG1})
	cotire_init_compile_cmd(_cmd "${_option_LANGUAGE}" "${_option_COMPILER_EXECUTABLE}" "${_option_COMPILER_ARG1}")
	cotire_add_definitions_to_cmd(_cmd "${_option_LANGUAGE}" ${_option_COMPILE_DEFINITIONS})
	cotire_add_compile_flags_to_cmd(_cmd ${_option_COMPILE_FLAGS})
	cotire_add_includes_to_cmd(_cmd "${_option_LANGUAGE}" ${_option_INCLUDE_DIRECTORIES})
	cotire_add_frameworks_to_cmd(_cmd "${_option_LANGUAGE}" ${_option_INCLUDE_DIRECTORIES})
	cotire_add_makedep_flags("${_option_LANGUAGE}" "${_option_COMPILER_ID}" "${_option_COMPILER_VERSION}" _cmd)
	# only consider existing source files for scanning
	set (_existingSourceFiles "")
	foreach (_sourceFile ${_sourceFiles})
		if (EXISTS "${_sourceFile}")
			list (APPEND _existingSourceFiles "${_sourceFile}")
		endif()
	endforeach()
	if (NOT _existingSourceFiles)
		set (${_includesVar} "" PARENT_SCOPE)
		return()
	endif()
	list (APPEND _cmd ${_existingSourceFiles})
	if (COTIRE_VERBOSE)
		message (STATUS "execute_process: ${_cmd}")
	endif()
	if (_option_COMPILER_ID MATCHES "MSVC")
		if (COTIRE_DEBUG)
			message (STATUS "clearing VS_UNICODE_OUTPUT")
		endif()
		# cl.exe messes with the output streams unless the environment variable VS_UNICODE_OUTPUT is cleared
		unset (ENV{VS_UNICODE_OUTPUT})
	endif()
	execute_process(COMMAND ${_cmd} WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
		RESULT_VARIABLE _result OUTPUT_QUIET ERROR_VARIABLE _output)
	if (_result)
		message (STATUS "Result ${_result} scanning includes of ${_existingSourceFiles}.")
	endif()
	cotire_parse_includes(
		"${_option_LANGUAGE}" "${_output}"
		"${_option_IGNORE_PATH}" "${_option_INCLUDE_PATH}"
		"${_option_IGNORE_EXTENSIONS}"
		_includes _unparsedLines
		${_sourceFiles})
	set (${_includesVar} ${_includes} PARENT_SCOPE)
	if (_option_UNPARSED_LINES)
		set (${_option_UNPARSED_LINES} ${_unparsedLines} PARENT_SCOPE)
	endif()
endfunction()

macro (cotire_append_undefs _contentsVar)
	set (_undefs ${ARGN})
	if (_undefs)
		list (REMOVE_DUPLICATES _undefs)
		foreach (_definition ${_undefs})
			list (APPEND ${_contentsVar} "#undef ${_definition}")
		endforeach()
	endif()
endmacro()

macro (cotire_comment_str _language _commentText _commentVar)
	if ("${_language}" STREQUAL "CMAKE")
		set (${_commentVar} "# ${_commentText}")
	else()
		set (${_commentVar} "/* ${_commentText} */")
	endif()
endmacro()

function (cotire_write_file _language _file _contents _force)
	get_filename_component(_moduleName "${COTIRE_CMAKE_MODULE_FILE}" NAME)
	cotire_comment_str("${_language}" "${_moduleName} ${COTIRE_CMAKE_MODULE_VERSION} generated file" _header1)
	cotire_comment_str("${_language}" "${_file}" _header2)
	set (_contents "${_header1}\n${_header2}\n${_contents}")
	if (COTIRE_DEBUG)
		message (STATUS "${_contents}")
	endif()
	if (_force OR NOT EXISTS "${_file}")
		file (WRITE "${_file}" "${_contents}")
	else()
		file (READ "${_file}" _oldContents)
		if (NOT "${_oldContents}" STREQUAL "${_contents}")
			file (WRITE "${_file}" "${_contents}")
		else()
			if (COTIRE_DEBUG)
				message (STATUS "${_file} unchanged")
			endif()
		endif()
	endif()
endfunction()

function (cotire_generate_unity_source _unityFile)
	set(_options "")
	set(_oneValueArgs LANGUAGE)
	set(_multiValueArgs
		DEPENDS SOURCES_COMPILE_DEFINITIONS
		PRE_UNDEFS SOURCES_PRE_UNDEFS POST_UNDEFS SOURCES_POST_UNDEFS PROLOGUE EPILOGUE)
	cmake_parse_arguments(_option "${_options}" "${_oneValueArgs}" "${_multiValueArgs}" ${ARGN})
	if (_option_DEPENDS)
		cotire_check_file_up_to_date(_unityFileIsUpToDate "${_unityFile}" ${_option_DEPENDS})
		if (_unityFileIsUpToDate)
			return()
		endif()
	endif()
	set (_sourceFiles ${_option_UNPARSED_ARGUMENTS})
	if (NOT _option_PRE_UNDEFS)
		set (_option_PRE_UNDEFS "")
	endif()
	if (NOT _option_SOURCES_PRE_UNDEFS)
		set (_option_SOURCES_PRE_UNDEFS "")
	endif()
	if (NOT _option_POST_UNDEFS)
		set (_option_POST_UNDEFS "")
	endif()
	if (NOT _option_SOURCES_POST_UNDEFS)
		set (_option_SOURCES_POST_UNDEFS "")
	endif()
	set (_contents "")
	if (_option_PROLOGUE)
		list (APPEND _contents ${_option_PROLOGUE})
	endif()
	if (_option_LANGUAGE AND _sourceFiles)
		if ("${_option_LANGUAGE}" STREQUAL "CXX")
			list (APPEND _contents "#ifdef __cplusplus")
		elseif ("${_option_LANGUAGE}" STREQUAL "C")
			list (APPEND _contents "#ifndef __cplusplus")
		endif()
	endif()
	set (_compileUndefinitions "")
	foreach (_sourceFile ${_sourceFiles})
		cotire_get_source_compile_definitions(
			"${_option_CONFIGURATION}" "${_option_LANGUAGE}" "${_sourceFile}" _compileDefinitions
			${_option_SOURCES_COMPILE_DEFINITIONS})
		cotire_get_source_undefs("${_sourceFile}" COTIRE_UNITY_SOURCE_PRE_UNDEFS _sourcePreUndefs ${_option_SOURCES_PRE_UNDEFS})
		cotire_get_source_undefs("${_sourceFile}" COTIRE_UNITY_SOURCE_POST_UNDEFS _sourcePostUndefs ${_option_SOURCES_POST_UNDEFS})
		if (_option_PRE_UNDEFS)
			list (APPEND _compileUndefinitions ${_option_PRE_UNDEFS})
		endif()
		if (_sourcePreUndefs)
			list (APPEND _compileUndefinitions ${_sourcePreUndefs})
		endif()
		if (_compileUndefinitions)
			cotire_append_undefs(_contents ${_compileUndefinitions})
			set (_compileUndefinitions "")
		endif()
		if (_sourcePostUndefs)
			list (APPEND _compileUndefinitions ${_sourcePostUndefs})
		endif()
		if (_option_POST_UNDEFS)
			list (APPEND _compileUndefinitions ${_option_POST_UNDEFS})
		endif()
		foreach (_definition ${_compileDefinitions})
			if (_definition MATCHES "^([a-zA-Z0-9_]+)=(.+)$")
				list (APPEND _contents "#define ${CMAKE_MATCH_1} ${CMAKE_MATCH_2}")
				list (INSERT _compileUndefinitions 0 "${CMAKE_MATCH_1}")
			else()
				list (APPEND _contents "#define ${_definition}")
				list (INSERT _compileUndefinitions 0 "${_definition}")
			endif()
		endforeach()
		get_filename_component(_sourceFile "${_sourceFile}" ABSOLUTE)
		if (WIN32)
			file (TO_NATIVE_PATH "${_sourceFile}" _sourceFile)
		endif()
		list (APPEND _contents "#include \"${_sourceFile}\"")
	endforeach()
	if (_compileUndefinitions)
		cotire_append_undefs(_contents ${_compileUndefinitions})
		set (_compileUndefinitions "")
	endif()
	if (_option_LANGUAGE AND _sourceFiles)
		list (APPEND _contents "#endif")
	endif()
	if (_option_EPILOGUE)
		list (APPEND _contents ${_option_EPILOGUE})
	endif()
	list (APPEND _contents "")
	string (REPLACE ";" "\n" _contents "${_contents}")
	if (COTIRE_VERBOSE)
		message ("${_contents}")
	endif()
	cotire_write_file("${_option_LANGUAGE}" "${_unityFile}" "${_contents}" TRUE)
endfunction()

function (cotire_generate_prefix_header _prefixFile)
	set(_options "")
	set(_oneValueArgs LANGUAGE COMPILER_EXECUTABLE COMPILER_ID COMPILER_VERSION)
	set(_multiValueArgs DEPENDS COMPILE_DEFINITIONS COMPILE_FLAGS
		INCLUDE_DIRECTORIES IGNORE_PATH INCLUDE_PATH IGNORE_EXTENSIONS)
	cmake_parse_arguments(_option "${_options}" "${_oneValueArgs}" "${_multiValueArgs}" ${ARGN})
	if (_option_DEPENDS)
		cotire_check_file_up_to_date(_prefixFileIsUpToDate "${_prefixFile}" ${_option_DEPENDS})
		if (_prefixFileIsUpToDate)
			return()
		endif()
	endif()
	set (_prologue "")
	set (_epilogue "")
	if (_option_COMPILER_ID MATCHES "Clang")
		set (_prologue "#pragma clang system_header")
	elseif (_option_COMPILER_ID MATCHES "GNU")
		set (_prologue "#pragma GCC system_header")
	elseif (_option_COMPILER_ID MATCHES "MSVC")
		set (_prologue "#pragma warning(push, 0)")
		set (_epilogue "#pragma warning(pop)")
	elseif (_option_COMPILER_ID MATCHES "Intel")
		# Intel compiler requires hdrstop pragma to stop generating PCH file
		set (_epilogue "#pragma hdrstop")
	endif()
	set (_sourceFiles ${_option_UNPARSED_ARGUMENTS})
	cotire_scan_includes(_selectedHeaders ${_sourceFiles}
		LANGUAGE "${_option_LANGUAGE}"
		COMPILER_EXECUTABLE "${_option_COMPILER_EXECUTABLE}"
		COMPILER_ID "${_option_COMPILER_ID}"
		COMPILER_VERSION "${_option_COMPILER_VERSION}"
		COMPILE_DEFINITIONS ${_option_COMPILE_DEFINITIONS}
		COMPILE_FLAGS ${_option_COMPILE_FLAGS}
		INCLUDE_DIRECTORIES ${_option_INCLUDE_DIRECTORIES}
		IGNORE_PATH ${_option_IGNORE_PATH}
		INCLUDE_PATH ${_option_INCLUDE_PATH}
		IGNORE_EXTENSIONS ${_option_IGNORE_EXTENSIONS}
		UNPARSED_LINES _unparsedLines)
	cotire_generate_unity_source("${_prefixFile}"
		PROLOGUE ${_prologue} EPILOGUE ${_epilogue} LANGUAGE "${_option_LANGUAGE}" ${_selectedHeaders})
	set (_unparsedLinesFile "${_prefixFile}.log")
	if (_unparsedLines)
		if (COTIRE_VERBOSE OR NOT _selectedHeaders)
			list (LENGTH _unparsedLines _skippedLineCount)
			file (RELATIVE_PATH _unparsedLinesFileRelPath "${CMAKE_BINARY_DIR}" "${_unparsedLinesFile}")
			message (STATUS "${_skippedLineCount} line(s) skipped, see ${_unparsedLinesFileRelPath}")
		endif()
		string (REPLACE ";" "\n" _unparsedLines "${_unparsedLines}")
	endif()
	file (WRITE "${_unparsedLinesFile}" "${_unparsedLines}")
endfunction()

function (cotire_add_makedep_flags _language _compilerID _compilerVersion _flagsVar)
	set (_flags ${${_flagsVar}})
	if (_compilerID MATCHES "MSVC")
		# cl.exe options used
		# /nologo suppresses display of sign-on banner
		# /TC treat all files named on the command line as C source files
		# /TP treat all files named on the command line as C++ source files
		# /EP preprocess to stdout without #line directives
		# /showIncludes list include files
		set (_sourceFileTypeC "/TC")
		set (_sourceFileTypeCXX "/TP")
		if (_flags)
			# append to list
			list (APPEND _flags /nologo "${_sourceFileType${_language}}" /EP /showIncludes)
		else()
			# return as a flag string
			set (_flags "${_sourceFileType${_language}} /EP /showIncludes")
		endif()
	elseif (_compilerID MATCHES "GNU")
		# GCC options used
		# -H print the name of each header file used
		# -E invoke preprocessor
		# -fdirectives-only do not expand macros, requires GCC >= 4.3
		if (_flags)
			# append to list
			list (APPEND _flags -H -E)
			if (NOT "${_compilerVersion}" VERSION_LESS "4.3.0")
				list (APPEND _flags "-fdirectives-only")
			endif()
		else()
			# return as a flag string
			set (_flags "-H -E")
			if (NOT "${_compilerVersion}" VERSION_LESS "4.3.0")
				set (_flags "${_flags} -fdirectives-only")
			endif()
		endif()
	elseif (_compilerID MATCHES "Clang")
		# Clang options used
		# -H print the name of each header file used
		# -E invoke preprocessor
		if (_flags)
			# append to list
			list (APPEND _flags -H -E)
		else()
			# return as a flag string
			set (_flags "-H -E")
		endif()
	elseif (_compilerID MATCHES "Intel")
		if (WIN32)
			# Windows Intel options used
			# /nologo do not display compiler version information
			# /QH display the include file order
			# /EP preprocess to stdout, omitting #line directives
			# /TC process all source or unrecognized file types as C source files
			# /TP process all source or unrecognized file types as C++ source files
			set (_sourceFileTypeC "/TC")
			set (_sourceFileTypeCXX "/TP")
			if (_flags)
				# append to list
				list (APPEND _flags /nologo "${_sourceFileType${_language}}" /EP /QH)
			else()
				# return as a flag string
				set (_flags "${_sourceFileType${_language}} /EP /QH")
			endif()
		else()
			# Linux / Mac OS X Intel options used
			# -H print the name of each header file used
			# -EP preprocess to stdout, omitting #line directives
			# -Kc++ process all source or unrecognized file types as C++ source files
			if (_flags)
				# append to list
				if ("${_language}" STREQUAL "CXX")
					list (APPEND _flags -Kc++)
				endif()
				list (APPEND _flags -H -EP)
			else()
				# return as a flag string
				if ("${_language}" STREQUAL "CXX")
					set (_flags "-Kc++ ")
				endif()
				set (_flags "${_flags}-H -EP")
			endif()
		endif()
	else()
		message (FATAL_ERROR "Unsupported ${_language} compiler ${_compilerID} version ${_compilerVersion}.")
	endif()
	set (${_flagsVar} ${_flags} PARENT_SCOPE)
endfunction()

function (cotire_add_pch_compilation_flags _language _compilerID _compilerVersion _prefixFile _pchFile _hostFile _flagsVar)
	set (_flags ${${_flagsVar}})
	if (_compilerID MATCHES "MSVC")
		file (TO_NATIVE_PATH "${_prefixFile}" _prefixFileNative)
		file (TO_NATIVE_PATH "${_pchFile}" _pchFileNative)
		file (TO_NATIVE_PATH "${_hostFile}" _hostFileNative)
		# cl.exe options used
		# /Yc creates a precompiled header file
		# /Fp specifies precompiled header binary file name
		# /FI forces inclusion of file
		# /TC treat all files named on the command line as C source files
		# /TP treat all files named on the command line as C++ source files
		# /Zs syntax check only
		set (_sourceFileTypeC "/TC")
		set (_sourceFileTypeCXX "/TP")
		if (_flags)
			# append to list
			list (APPEND _flags /nologo "${_sourceFileType${_language}}"
				"/Yc${_prefixFileNative}" "/Fp${_pchFileNative}" "/FI${_prefixFileNative}" /Zs "${_hostFileNative}")
		else()
			# return as a flag string
			set (_flags "/Yc\"${_prefixFileNative}\" /Fp\"${_pchFileNative}\" /FI\"${_prefixFileNative}\"")
		endif()
	elseif (_compilerID MATCHES "GNU|Clang")
		# GCC / Clang options used
		# -w disable all warnings
		# -x specify the source language
		# -c compile but do not link
		# -o place output in file
		set (_xLanguage_C "c-header")
		set (_xLanguage_CXX "c++-header")
		if (_flags)
			# append to list
			list (APPEND _flags "-w" "-x" "${_xLanguage_${_language}}" "-c" "${_prefixFile}" -o "${_pchFile}")
		else()
			# return as a flag string
			set (_flags "-w -x ${_xLanguage_${_language}} -c \"${_prefixFile}\" -o \"${_pchFile}\"")
		endif()
	elseif (_compilerID MATCHES "Intel")
		if (WIN32)
			file (TO_NATIVE_PATH "${_prefixFile}" _prefixFileNative)
			file (TO_NATIVE_PATH "${_pchFile}" _pchFileNative)
			file (TO_NATIVE_PATH "${_hostFile}" _hostFileNative)
			# Windows Intel options used
			# /nologo do not display compiler version information
			# /Yc create a precompiled header (PCH) file
			# /Fp specify a path or file name for precompiled header files
			# /FI tells the preprocessor to include a specified file name as the header file
			# /TC process all source or unrecognized file types as C source files
			# /TP process all source or unrecognized file types as C++ source files
			# /Zs syntax check only
			# /Wpch-messages enable diagnostics related to pre-compiled headers (requires Intel XE 2013 Update 2)
			set (_sourceFileTypeC "/TC")
			set (_sourceFileTypeCXX "/TP")
			if (_flags)
				# append to list
				list (APPEND _flags /nologo "${_sourceFileType${_language}}"
					"/Yc" "/Fp${_pchFileNative}" "/FI${_prefixFileNative}" /Zs "${_hostFileNative}")
				if (NOT "${_compilerVersion}" VERSION_LESS "13.1.0")
					list (APPEND _flags "/Wpch-messages")
				endif()
			else()
				# return as a flag string
				set (_flags "/Yc /Fp\"${_pchFileNative}\" /FI\"${_prefixFileNative}\"")
				if (NOT "${_compilerVersion}" VERSION_LESS "13.1.0")
					set (_flags "${_flags} /Wpch-messages")
				endif()
			endif()
		else()
			# Linux / Mac OS X Intel options used
			# -pch-dir location for precompiled header files
			# -pch-create name of the precompiled header (PCH) to create
			# -Kc++ process all source or unrecognized file types as C++ source files
			# -fsyntax-only check only for correct syntax
			# -Wpch-messages enable diagnostics related to pre-compiled headers (requires Intel XE 2013 Update 2)
			get_filename_component(_pchDir "${_pchFile}" PATH)
			get_filename_component(_pchName "${_pchFile}" NAME)
			set (_xLanguage_C "c-header")
			set (_xLanguage_CXX "c++-header")
			if (_flags)
				# append to list
				if ("${_language}" STREQUAL "CXX")
					list (APPEND _flags -Kc++)
				endif()
				list (APPEND _flags "-include" "${_prefixFile}" "-pch-dir" "${_pchDir}" "-pch-create" "${_pchName}" "-fsyntax-only" "${_hostFile}")
				if (NOT "${_compilerVersion}" VERSION_LESS "13.1.0")
					list (APPEND _flags "-Wpch-messages")
				endif()
			else()
				# return as a flag string
				set (_flags "-include \"${_prefixFile}\" -pch-dir \"${_pchDir}\" -pch-create \"${_pchName}\"")
				if (NOT "${_compilerVersion}" VERSION_LESS "13.1.0")
					set (_flags "${_flags} -Wpch-messages")
				endif()
			endif()
		endif()
	else()
		message (FATAL_ERROR "Unsupported ${_language} compiler ${_compilerID} version ${_compilerVersion}.")
	endif()
	set (${_flagsVar} ${_flags} PARENT_SCOPE)
endfunction()

function (cotire_add_prefix_pch_inclusion_flags _language _compilerID _compilerVersion _prefixFile _pchFile _flagsVar)
	set (_flags ${${_flagsVar}})
	if (_compilerID MATCHES "MSVC")
		file (TO_NATIVE_PATH "${_prefixFile}" _prefixFileNative)
		# cl.exe options used
		# /Yu uses a precompiled header file during build
		# /Fp specifies precompiled header binary file name
		# /FI forces inclusion of file
		if (_pchFile)
			file (TO_NATIVE_PATH "${_pchFile}" _pchFileNative)
			if (_flags)
				# append to list
				list (APPEND _flags "/Yu${_prefixFileNative}" "/Fp${_pchFileNative}" "/FI${_prefixFileNative}")
			else()
				# return as a flag string
				set (_flags "/Yu\"${_prefixFileNative}\" /Fp\"${_pchFileNative}\" /FI\"${_prefixFileNative}\"")
			endif()
		else()
			# no precompiled header, force inclusion of prefix header
			if (_flags)
				# append to list
				list (APPEND _flags "/FI${_prefixFileNative}")
			else()
				# return as a flag string
				set (_flags "/FI\"${_prefixFileNative}\"")
			endif()
		endif()
	elseif (_compilerID MATCHES "GNU")
		# GCC options used
		# -include process include file as the first line of the primary source file
		# -Winvalid-pch warns if precompiled header is found but cannot be used
		if (_flags)
			# append to list
			list (APPEND _flags "-Winvalid-pch" "-include" "${_prefixFile}")
		else()
			# return as a flag string
			set (_flags "-Winvalid-pch -include \"${_prefixFile}\"")
		endif()
	elseif (_compilerID MATCHES "Clang")
		# Clang options used
		# -include process include file as the first line of the primary source file
		# -include-pch include precompiled header file
		# -Qunused-arguments don't emit warning for unused driver arguments
		if (_pchFile AND NOT CMAKE_${_language}_COMPILER MATCHES "ccache")
			if (_flags)
				# append to list
					list (APPEND _flags "-Qunused-arguments" "-include-pch" "${_pchFile}")
			else()
				# return as a flag string
				set (_flags "-Qunused-arguments -include-pch \"${_pchFile}\"")
			endif()
		else()
			# no precompiled header, force inclusion of prefix header
			# ccache requires the -include flag to be used in order to process precompiled header correctly
			if (_flags)
				# append to list
				list (APPEND _flags "-Qunused-arguments" "-include" "${_prefixFile}")
			else()
				# return as a flag string
				set (_flags "-Qunused-arguments -include \"${_prefixFile}\"")
			endif()
		endif()
	elseif (_compilerID MATCHES "Intel")
		if (WIN32)
			file (TO_NATIVE_PATH "${_prefixFile}" _prefixFileNative)
			# Windows Intel options used
			# /Yu use a precompiled header (PCH) file
			# /Fp specify a path or file name for precompiled header files
			# /FI tells the preprocessor to include a specified file name as the header file
			# /Wpch-messages enable diagnostics related to pre-compiled headers (requires Intel XE 2013 Update 2)
			if (_pchFile)
				file (TO_NATIVE_PATH "${_pchFile}" _pchFileNative)
				if (_flags)
					# append to list
					list (APPEND _flags "/Yu" "/Fp${_pchFileNative}" "/FI${_prefixFileNative}")
					if (NOT "${_compilerVersion}" VERSION_LESS "13.1.0")
						list (APPEND _flags "/Wpch-messages")
					endif()
				else()
					# return as a flag string
					set (_flags "/Yu /Fp\"${_pchFileNative}\" /FI\"${_prefixFileNative}\"")
					if (NOT "${_compilerVersion}" VERSION_LESS "13.1.0")
						set (_flags "${_flags} /Wpch-messages")
					endif()
				endif()
			else()
				# no precompiled header, force inclusion of prefix header
				if (_flags)
					# append to list
					list (APPEND _flags "/FI${_prefixFileNative}")
				else()
					# return as a flag string
					set (_flags "/FI\"${_prefixFileNative}\"")
				endif()
			endif()
		else()
			# Linux / Mac OS X Intel options used
			# -pch-dir location for precompiled header files
			# -pch-use name of the precompiled header (PCH) to use
			# -include process include file as the first line of the primary source file
			# -Wpch-messages enable diagnostics related to pre-compiled headers (requires Intel XE 2013 Update 2)
			if (_pchFile)
				get_filename_component(_pchDir "${_pchFile}" PATH)
				get_filename_component(_pchName "${_pchFile}" NAME)
				if (_flags)
					# append to list
					list (APPEND _flags "-include" "${_prefixFile}" "-pch-dir" "${_pchDir}" "-pch-use" "${_pchName}")
					if (NOT "${_compilerVersion}" VERSION_LESS "13.1.0")
						list (APPEND _flags "-Wpch-messages")
					endif()
				else()
					# return as a flag string
					set (_flags "-include \"${_prefixFile}\" -pch-dir \"${_pchDir}\" -pch-use \"${_pchName}\"")
					if (NOT "${_compilerVersion}" VERSION_LESS "13.1.0")
						set (_flags "${_flags} -Wpch-messages")
					endif()
				endif()
			else()
				# no precompiled header, force inclusion of prefix header
				if (_flags)
					# append to list
					list (APPEND _flags "-include" "${_prefixFile}")
				else()
					# return as a flag string
					set (_flags "-include \"${_prefixFile}\"")
				endif()
			endif()
		endif()
	else()
		message (FATAL_ERROR "Unsupported ${_language} compiler ${_compilerID} version ${_compilerVersion}.")
	endif()
	set (${_flagsVar} ${_flags} PARENT_SCOPE)
endfunction()

function (cotire_precompile_prefix_header _prefixFile _pchFile _hostFile)
	set(_options "")
	set(_oneValueArgs COMPILER_EXECUTABLE COMPILER_ID COMPILER_VERSION LANGUAGE)
	set(_multiValueArgs COMPILE_DEFINITIONS COMPILE_FLAGS INCLUDE_DIRECTORIES)
	cmake_parse_arguments(_option "${_options}" "${_oneValueArgs}" "${_multiValueArgs}" ${ARGN})
	if (NOT _option_LANGUAGE)
		set (_option_LANGUAGE "CXX")
	endif()
	if (NOT _option_COMPILER_ID)
		set (_option_COMPILER_ID "${CMAKE_${_option_LANGUAGE}_ID}")
	endif()
	cotire_init_compile_cmd(_cmd "${_option_LANGUAGE}" "${_option_COMPILER_EXECUTABLE}" "${_option_COMPILER_ARG1}")
	cotire_add_definitions_to_cmd(_cmd "${_option_LANGUAGE}" ${_option_COMPILE_DEFINITIONS})
	cotire_add_compile_flags_to_cmd(_cmd ${_option_COMPILE_FLAGS})
	cotire_add_includes_to_cmd(_cmd "${_option_LANGUAGE}" ${_option_INCLUDE_DIRECTORIES})
	cotire_add_frameworks_to_cmd(_cmd "${_option_LANGUAGE}" ${_option_INCLUDE_DIRECTORIES})
	cotire_add_pch_compilation_flags(
		"${_option_LANGUAGE}" "${_option_COMPILER_ID}" "${_option_COMPILER_VERSION}"
		"${_prefixFile}" "${_pchFile}" "${_hostFile}" _cmd)
	if (COTIRE_VERBOSE)
		message (STATUS "execute_process: ${_cmd}")
	endif()
	if (_option_COMPILER_ID MATCHES "MSVC")
		if (COTIRE_DEBUG)
			message (STATUS "clearing VS_UNICODE_OUTPUT")
		endif()
		# cl.exe messes with the output streams unless the environment variable VS_UNICODE_OUTPUT is cleared
		unset (ENV{VS_UNICODE_OUTPUT})
	endif()
	execute_process(COMMAND ${_cmd} WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" RESULT_VARIABLE _result)
	if (_result)
		message (FATAL_ERROR "Error ${_result} precompiling ${_prefixFile}.")
	endif()
endfunction()

function (cotire_check_precompiled_header_support _language _targetSourceDir _target _msgVar)
	set (_unsupportedCompiler
		"Precompiled headers not supported for ${_language} compiler ${CMAKE_${_language}_COMPILER_ID}")
	if (CMAKE_${_language}_COMPILER_ID MATCHES "MSVC")
		# supported since Visual Studio C++ 6.0
		# and CMake does not support an earlier version
		set (${_msgVar} "" PARENT_SCOPE)
	elseif (CMAKE_${_language}_COMPILER_ID MATCHES "GNU")
		# GCC PCH support requires version >= 3.4
		cotire_determine_compiler_version("${_language}" COTIRE_${_language}_COMPILER)
		if ("${COTIRE_${_language}_COMPILER_VERSION}" MATCHES ".+" AND
			"${COTIRE_${_language}_COMPILER_VERSION}" VERSION_LESS "3.4.0")
			set (${_msgVar} "${_unsupportedCompiler} version ${COTIRE_${_language}_COMPILER_VERSION}." PARENT_SCOPE)
		else()
			set (${_msgVar} "" PARENT_SCOPE)
		endif()
	elseif (CMAKE_${_language}_COMPILER_ID MATCHES "Clang")
		# all Clang versions have PCH support
		set (${_msgVar} "" PARENT_SCOPE)
	elseif (CMAKE_${_language}_COMPILER_ID MATCHES "Intel")
		# Intel PCH support requires version >= 8.0.0
		cotire_determine_compiler_version("${_language}" COTIRE_${_language}_COMPILER)
		if ("${COTIRE_${_language}_COMPILER_VERSION}" MATCHES ".+" AND
			"${COTIRE_${_language}_COMPILER_VERSION}" VERSION_LESS "8.0.0")
			set (${_msgVar} "${_unsupportedCompiler} version ${COTIRE_${_language}_COMPILER_VERSION}." PARENT_SCOPE)
		else()
			set (${_msgVar} "" PARENT_SCOPE)
		endif()
	else()
		set (${_msgVar} "${_unsupportedCompiler}." PARENT_SCOPE)
	endif()
	if (CMAKE_${_language}_COMPILER MATCHES "ccache")
		if (NOT "$ENV{CCACHE_SLOPPINESS}" MATCHES "time_macros")
			set (${_msgVar}
				"ccache requires the environment variable CCACHE_SLOPPINESS to be set to time_macros."
				PARENT_SCOPE)
		endif()
	endif()
	if (APPLE)
		# PCH compilation not supported by GCC / Clang for multi-architecture builds (e.g., i386, x86_64)
		if (CMAKE_CONFIGURATION_TYPES)
			set (_configs ${CMAKE_CONFIGURATION_TYPES})
		elseif (CMAKE_BUILD_TYPE)
			set (_configs ${CMAKE_BUILD_TYPE})
		else()
			set (_configs "None")
		endif()
		foreach (_config ${_configs})
			set (_targetFlags "")
			cotire_get_target_compile_flags("${_config}" "${_language}" "${_targetSourceDir}" "${_target}" _targetFlags)
			cotire_filter_compile_flags("${_language}" "arch" _architectures _ignore ${_targetFlags})
			list (LENGTH _architectures _numberOfArchitectures)
			if (_numberOfArchitectures GREATER 1)
				string (REPLACE ";" ", " _architectureStr "${_architectures}")
				set (${_msgVar}
					"Precompiled headers not supported on Darwin for multi-architecture builds (${_architectureStr})."
					PARENT_SCOPE)
				break()
			endif()
		endforeach()
	endif()
endfunction()

macro (cotire_get_intermediate_dir _cotireDir)
	get_filename_component(${_cotireDir} "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/${COTIRE_INTDIR}" ABSOLUTE)
endmacro()

macro (cotire_setup_file_extension_variables)
	set (_unityFileExt_C ".c")
	set (_unityFileExt_CXX ".cxx")
	set (_prefixFileExt_C ".h")
	set (_prefixFileExt_CXX ".hxx")
endmacro()

function (cotire_make_single_unity_source_file_path _language _target _unityFileVar)
	cotire_setup_file_extension_variables()
	if (NOT DEFINED _unityFileExt_${_language})
		set (${_unityFileVar} "" PARENT_SCOPE)
		return()
	endif()
	set (_unityFileBaseName "${_target}_${_language}${COTIRE_UNITY_SOURCE_FILENAME_SUFFIX}")
	set (_unityFileName "${_unityFileBaseName}${_unityFileExt_${_language}}")
	cotire_get_intermediate_dir(_baseDir)
	set (_unityFile "${_baseDir}/${_unityFileName}")
	set (${_unityFileVar} "${_unityFile}" PARENT_SCOPE)
	if (COTIRE_DEBUG)
		message(STATUS "${_unityFile}")
	endif()
endfunction()

function (cotire_make_unity_source_file_paths _language _target _maxIncludes _unityFilesVar)
	cotire_setup_file_extension_variables()
	if (NOT DEFINED _unityFileExt_${_language})
		set (${_unityFileVar} "" PARENT_SCOPE)
		return()
	endif()
	set (_unityFileBaseName "${_target}_${_language}${COTIRE_UNITY_SOURCE_FILENAME_SUFFIX}")
	cotire_get_intermediate_dir(_baseDir)
	set (_startIndex 0)
	set (_index 0)
	set (_unityFiles "")
	set (_sourceFiles ${ARGN})
	foreach (_sourceFile ${_sourceFiles})
		get_source_file_property(_startNew "${_sourceFile}" COTIRE_START_NEW_UNITY_SOURCE)
		math (EXPR _unityFileCount "${_index} - ${_startIndex}")
		if (_startNew OR (_maxIncludes GREATER 0 AND NOT _unityFileCount LESS _maxIncludes))
			if (_index GREATER 0)
				# start new unity file segment
				math (EXPR _endIndex "${_index} - 1")
				set (_unityFileName "${_unityFileBaseName}_${_startIndex}_${_endIndex}${_unityFileExt_${_language}}")
				list (APPEND _unityFiles "${_baseDir}/${_unityFileName}")
			endif()
			set (_startIndex ${_index})
		endif()
		math (EXPR _index "${_index} + 1")
	endforeach()
	list (LENGTH _sourceFiles _numberOfSources)
	if (_startIndex EQUAL 0)
		# there is only a single unity file
		cotire_make_single_unity_source_file_path(${_language} ${_target} _unityFiles)
	elseif (_startIndex LESS _numberOfSources)
		# end with final unity file segment
		math (EXPR _endIndex "${_index} - 1")
		set (_unityFileName "${_unityFileBaseName}_${_startIndex}_${_endIndex}${_unityFileExt_${_language}}")
		list (APPEND _unityFiles "${_baseDir}/${_unityFileName}")
	endif()
	set (${_unityFilesVar} ${_unityFiles} PARENT_SCOPE)
	if (COTIRE_DEBUG)
		message(STATUS "${_unityFiles}")
	endif()
endfunction()

function (cotire_unity_to_prefix_file_path _language _target _unityFile _prefixFileVar)
	cotire_setup_file_extension_variables()
	if (NOT DEFINED _unityFileExt_${_language})
		set (${_prefixFileVar} "" PARENT_SCOPE)
		return()
	endif()
	set (_unityFileBaseName "${_target}_${_language}${COTIRE_UNITY_SOURCE_FILENAME_SUFFIX}")
	set (_prefixFileBaseName "${_target}_${_language}${COTIRE_PREFIX_HEADER_FILENAME_SUFFIX}")
	string (REPLACE "${_unityFileBaseName}" "${_prefixFileBaseName}" _prefixFile "${_unityFile}")
	string (REGEX REPLACE "${_unityFileExt_${_language}}$" "${_prefixFileExt_${_language}}" _prefixFile "${_prefixFile}")
	set (${_prefixFileVar} "${_prefixFile}" PARENT_SCOPE)
endfunction()

function (cotire_make_prefix_file_name _language _target _prefixFileBaseNameVar _prefixFileNameVar)
	cotire_setup_file_extension_variables()
	if (NOT _language)
		set (_prefixFileBaseName "${_target}${COTIRE_PREFIX_HEADER_FILENAME_SUFFIX}")
		set (_prefixFileName "${_prefixFileBaseName}${_prefixFileExt_C}")
	elseif (DEFINED _prefixFileExt_${_language})
		set (_prefixFileBaseName "${_target}_${_language}${COTIRE_PREFIX_HEADER_FILENAME_SUFFIX}")
		set (_prefixFileName "${_prefixFileBaseName}${_prefixFileExt_${_language}}")
	else()
		set (_prefixFileBaseName "")
		set (_prefixFileName "")
	endif()
	set (${_prefixFileBaseNameVar} "${_prefixFileBaseName}" PARENT_SCOPE)
	set (${_prefixFileNameVar} "${_prefixFileName}" PARENT_SCOPE)
endfunction()

function (cotire_make_prefix_file_path _language _target _prefixFileVar)
	cotire_make_prefix_file_name("${_language}" "${_target}" _prefixFileBaseName _prefixFileName)
	set (${_prefixFileVar} "" PARENT_SCOPE)
	if (_prefixFileName)
		if (NOT _language)
			set (_language "C")
		endif()
		if (MSVC OR CMAKE_${_language}_COMPILER_ID MATCHES "GNU|Clang|Intel")
			cotire_get_intermediate_dir(_baseDir)
			set (${_prefixFileVar} "${_baseDir}/${_prefixFileName}" PARENT_SCOPE)
		endif()
	endif()
endfunction()

function (cotire_make_pch_file_path _language _targetSourceDir _target _pchFileVar)
	cotire_make_prefix_file_name("${_language}" "${_target}" _prefixFileBaseName _prefixFileName)
	set (${_pchFileVar} "" PARENT_SCOPE)
	if (_prefixFileBaseName AND _prefixFileName)
		cotire_check_precompiled_header_support("${_language}" "${_targetSourceDir}" "${_target}" _msg)
		if (NOT _msg)
			if (XCODE)
				# For Xcode, we completely hand off the compilation of the prefix header to the IDE
				return()
			endif()
			cotire_get_intermediate_dir(_baseDir)
			if (CMAKE_${_language}_COMPILER_ID MATCHES "MSVC")
				# MSVC uses the extension .pch added to the prefix header base name
				set (${_pchFileVar} "${_baseDir}/${_prefixFileBaseName}.pch" PARENT_SCOPE)
			elseif (CMAKE_${_language}_COMPILER_ID MATCHES "Clang")
				# Clang looks for a precompiled header corresponding to the prefix header with the extension .pch appended
				set (${_pchFileVar} "${_baseDir}/${_prefixFileName}.pch" PARENT_SCOPE)
			elseif (CMAKE_${_language}_COMPILER_ID MATCHES "GNU")
				# GCC looks for a precompiled header corresponding to the prefix header with the extension .gch appended
				set (${_pchFileVar} "${_baseDir}/${_prefixFileName}.gch" PARENT_SCOPE)
			elseif (CMAKE_${_language}_COMPILER_ID MATCHES "Intel")
				# Intel uses the extension .pchi added to the prefix header base name
				set (${_pchFileVar} "${_baseDir}/${_prefixFileBaseName}.pchi" PARENT_SCOPE)
			endif()
		endif()
	endif()
endfunction()

function (cotire_select_unity_source_files _unityFile _sourcesVar)
	set (_sourceFiles ${ARGN})
	if (_sourceFiles AND "${_unityFile}" MATCHES "${COTIRE_UNITY_SOURCE_FILENAME_SUFFIX}_([0-9]+)_([0-9]+)")
		set (_startIndex ${CMAKE_MATCH_1})
		set (_endIndex ${CMAKE_MATCH_2})
		list (LENGTH _sourceFiles _numberOfSources)
		if (NOT _startIndex LESS _numberOfSources)
			math (EXPR _startIndex "${_numberOfSources} - 1")
		endif()
		if (NOT _endIndex LESS _numberOfSources)
			math (EXPR _endIndex "${_numberOfSources} - 1")
		endif()
		set (_files "")
		foreach (_index RANGE ${_startIndex} ${_endIndex})
			list (GET _sourceFiles ${_index} _file)
			list (APPEND _files "${_file}")
		endforeach()
	else()
		set (_files ${_sourceFiles})
	endif()
	set (${_sourcesVar} ${_files} PARENT_SCOPE)
endfunction()

function (cotire_get_unity_source_dependencies _language _target _dependencySourcesVar)
	set (_dependencySources "")
	# depend on target's generated source files
	cotire_get_objects_with_property_on(_generatedSources GENERATED SOURCE ${ARGN})
	if (_generatedSources)
		# but omit all generated source files that have the COTIRE_EXCLUDED property set to true
		cotire_get_objects_with_property_on(_excludedGeneratedSources COTIRE_EXCLUDED SOURCE ${_generatedSources})
		if (_excludedGeneratedSources)
			list (REMOVE_ITEM _generatedSources ${_excludedGeneratedSources})
		endif()
		# and omit all generated source files that have the COTIRE_DEPENDENCY property set to false explicitly
		cotire_get_objects_with_property_off(_excludedNonDependencySources COTIRE_DEPENDENCY SOURCE ${_generatedSources})
		if (_excludedNonDependencySources)
			list (REMOVE_ITEM _generatedSources ${_excludedNonDependencySources})
		endif()
		if (_generatedSources)
			list (APPEND _dependencySources ${_generatedSources})
		endif()
	endif()
	if (COTIRE_DEBUG AND _dependencySources)
		message (STATUS "${_language} ${_target} unity source depends on ${_dependencySources}")
	endif()
	set (${_dependencySourcesVar} ${_dependencySources} PARENT_SCOPE)
endfunction()

function (cotire_get_prefix_header_dependencies _language _target _dependencySourcesVar)
	# depend on target source files marked with custom COTIRE_DEPENDENCY property
	set (_dependencySources "")
	cotire_get_objects_with_property_on(_dependencySources COTIRE_DEPENDENCY SOURCE ${ARGN})
	if (CMAKE_${_language}_COMPILER_ID MATCHES "Clang")
		# Clang raises a fatal error if a file is not found during preprocessing
		# thus we depend on target's generated source files for prefix header generation
		cotire_get_objects_with_property_on(_generatedSources GENERATED SOURCE ${ARGN})
		if (_generatedSources)
			list (APPEND _dependencySources ${_generatedSources})
		endif()
	endif()
	if (COTIRE_DEBUG AND _dependencySources)
		message (STATUS "${_language} ${_target} prefix header DEPENDS ${_dependencySources}")
	endif()
	set (${_dependencySourcesVar} ${_dependencySources} PARENT_SCOPE)
endfunction()

function (cotire_generate_target_script _language _configurations _targetSourceDir _targetBinaryDir _target _targetScriptVar)
	set (COTIRE_TARGET_SOURCES ${ARGN})
	get_filename_component(_moduleName "${COTIRE_CMAKE_MODULE_FILE}" NAME)
	set (_targetCotireScript "${CMAKE_CURRENT_BINARY_DIR}/${_target}_${_language}_${_moduleName}")
	cotire_get_prefix_header_dependencies(${_language} ${_target} COTIRE_TARGET_PREFIX_DEPENDS ${COTIRE_TARGET_SOURCES})
	cotire_get_unity_source_dependencies(${_language} ${_target} COTIRE_TARGET_UNITY_DEPENDS ${COTIRE_TARGET_SOURCES})
	# set up variables to be configured
	set (COTIRE_TARGET_LANGUAGE "${_language}")
	cotire_determine_compiler_version("${COTIRE_TARGET_LANGUAGE}" COTIRE_${_language}_COMPILER)
	get_target_property(COTIRE_TARGET_IGNORE_PATH ${_target} COTIRE_PREFIX_HEADER_IGNORE_PATH)
	cotire_add_sys_root_paths(COTIRE_TARGET_IGNORE_PATH)
	get_target_property(COTIRE_TARGET_INCLUDE_PATH ${_target} COTIRE_PREFIX_HEADER_INCLUDE_PATH)
	cotire_add_sys_root_paths(COTIRE_TARGET_INCLUDE_PATH)
	get_target_property(COTIRE_TARGET_PRE_UNDEFS ${_target} COTIRE_UNITY_SOURCE_PRE_UNDEFS)
	get_target_property(COTIRE_TARGET_POST_UNDEFS ${_target} COTIRE_UNITY_SOURCE_POST_UNDEFS)
	get_target_property(COTIRE_TARGET_MAXIMUM_NUMBER_OF_INCLUDES ${_target} COTIRE_UNITY_SOURCE_MAXIMUM_NUMBER_OF_INCLUDES)
	cotire_get_source_files_undefs(COTIRE_UNITY_SOURCE_PRE_UNDEFS COTIRE_TARGET_SOURCES_PRE_UNDEFS ${COTIRE_TARGET_SOURCES})
	cotire_get_source_files_undefs(COTIRE_UNITY_SOURCE_POST_UNDEFS COTIRE_TARGET_SOURCES_POST_UNDEFS ${COTIRE_TARGET_SOURCES})
	set (COTIRE_TARGET_CONFIGURATION_TYPES "${_configurations}")
	foreach (_config ${_configurations})
		string (TOUPPER "${_config}" _upperConfig)
		cotire_get_target_include_directories(
			"${_config}" "${_language}" "${_targetSourceDir}" "${_targetBinaryDir}" "${_target}" COTIRE_TARGET_INCLUDE_DIRECTORIES_${_upperConfig})
		cotire_get_target_compile_definitions(
			"${_config}" "${_language}" "${_targetSourceDir}" "${_target}" COTIRE_TARGET_COMPILE_DEFINITIONS_${_upperConfig})
		cotire_get_target_compiler_flags(
			"${_config}" "${_language}" "${_targetSourceDir}" "${_target}" COTIRE_TARGET_COMPILE_FLAGS_${_upperConfig})
		cotire_get_source_files_compile_definitions(
			"${_config}" "${_language}" COTIRE_TARGET_SOURCES_COMPILE_DEFINITIONS_${_upperConfig} ${COTIRE_TARGET_SOURCES})
	endforeach()
	get_cmake_property(_vars VARIABLES)
	string (REGEX MATCHALL "COTIRE_[A-Za-z0-9_]+" _matchVars "${_vars}")
	# remove COTIRE_VERBOSE which is passed as a CMake define on command line
	list (REMOVE_ITEM _matchVars COTIRE_VERBOSE)
	set (_contents "")
	foreach (_var IN LISTS _matchVars ITEMS
		MSVC CMAKE_GENERATOR CMAKE_BUILD_TYPE CMAKE_CONFIGURATION_TYPES
		CMAKE_${_language}_COMPILER_ID CMAKE_${_language}_COMPILER CMAKE_${_language}_COMPILER_ARG1
		CMAKE_${_language}_SOURCE_FILE_EXTENSIONS)
		if (DEFINED ${_var})
			string (REPLACE "\"" "\\\"" _value "${${_var}}")
			set (_contents "${_contents}set (${_var} \"${_value}\")\n")
		endif()
	endforeach()
	cotire_write_file("CMAKE" "${_targetCotireScript}" "${_contents}" FALSE)
	set (${_targetScriptVar} "${_targetCotireScript}" PARENT_SCOPE)
endfunction()

function (cotire_setup_pch_file_compilation _language _target _targetSourceDir _targetScript _prefixFile _pchFile)
	set (_sourceFiles ${ARGN})
	if (CMAKE_${_language}_COMPILER_ID MATCHES "MSVC|Intel")
		# for Visual Studio and Intel, we attach the precompiled header compilation to the first source file
		# the remaining files include the precompiled header, see cotire_setup_pch_file_inclusion
		if (_sourceFiles)
			file (TO_NATIVE_PATH "${_prefixFile}" _prefixFileNative)
			file (TO_NATIVE_PATH "${_pchFile}" _pchFileNative)
			list (GET _sourceFiles 0 _hostFile)
			set (_flags "")
			cotire_determine_compiler_version("${_language}" COTIRE_${_language}_COMPILER)
			cotire_add_pch_compilation_flags(
				"${_language}" "${CMAKE_${_language}_COMPILER_ID}" "${COTIRE_${_language}_COMPILER_VERSION}"
				"${_prefixFile}" "${_pchFile}" "${_hostFile}" _flags)
			set_property (SOURCE ${_hostFile} APPEND_STRING PROPERTY COMPILE_FLAGS " ${_flags} ")
			set_property (SOURCE ${_hostFile} APPEND PROPERTY OBJECT_OUTPUTS "${_pchFile}")
			# make first source file depend on prefix header
			set_property (SOURCE ${_hostFile} APPEND PROPERTY OBJECT_DEPENDS "${_prefixFile}")
			# mark first source file as cotired to prevent it from being used in another cotired target
			set_property (SOURCE ${_hostFile} PROPERTY COTIRE_TARGET "${_target}")
		endif()
	elseif ("${CMAKE_GENERATOR}" MATCHES "Makefiles|Ninja")
		# for makefile based generator, we add a custom command to precompile the prefix header
		if (_targetScript)
			cotire_set_cmd_to_prologue(_cmds)
			list (GET _sourceFiles 0 _hostFile)
			list (APPEND _cmds -P "${COTIRE_CMAKE_MODULE_FILE}" "precompile" "${_targetScript}" "${_prefixFile}" "${_pchFile}" "${_hostFile}")
			file (RELATIVE_PATH _pchFileRelPath "${CMAKE_BINARY_DIR}" "${_pchFile}")
			if (COTIRE_DEBUG)
				message (STATUS "add_custom_command: OUTPUT ${_pchFile} ${_cmds} DEPENDS ${_prefixFile} IMPLICIT_DEPENDS ${_language} ${_prefixFile}")
			endif()
			set_property (SOURCE "${_pchFile}" PROPERTY GENERATED TRUE)
			add_custom_command(OUTPUT "${_pchFile}"
				COMMAND ${_cmds}
				DEPENDS "${_prefixFile}"
				IMPLICIT_DEPENDS ${_language} "${_prefixFile}"
				WORKING_DIRECTORY "${_targetSourceDir}"
				COMMENT "Building ${_language} precompiled header ${_pchFileRelPath}" VERBATIM)
		endif()
	endif()
endfunction()

function (cotire_setup_pch_file_inclusion _language _target _wholeTarget _prefixFile _pchFile)
	set (_sourceFiles ${ARGN})
	if (CMAKE_${_language}_COMPILER_ID MATCHES "MSVC|Intel")
		# for Visual Studio and Intel, we include the precompiled header in all but the first source file
		# the first source file does the precompiled header compilation, see cotire_setup_pch_file_compilation
		list (LENGTH _sourceFiles _numberOfSourceFiles)
		if (_numberOfSourceFiles GREATER 1)
			# mark sources as cotired to prevent them from being used in another cotired target
			set_source_files_properties(${_sourceFiles} PROPERTIES COTIRE_TARGET "${_target}")
			list (REMOVE_AT _sourceFiles 0)
			set (_flags "")
			cotire_determine_compiler_version("${_language}" COTIRE_${_language}_COMPILER)
			cotire_add_prefix_pch_inclusion_flags(
				"${_language}" "${CMAKE_${_language}_COMPILER_ID}" "${COTIRE_${_language}_COMPILER_VERSION}"
				"${_prefixFile}" "${_pchFile}" _flags)
			set_property (SOURCE ${_sourceFiles} APPEND_STRING PROPERTY COMPILE_FLAGS " ${_flags} ")
			# make source files depend on precompiled header
			set_property (SOURCE ${_sourceFiles} APPEND PROPERTY OBJECT_DEPENDS "${_pchFile}")
		endif()
	elseif ("${CMAKE_GENERATOR}" MATCHES "Makefiles|Ninja")
		if (NOT _wholeTarget)
			# for makefile based generator, we force the inclusion of the prefix header for a subset
			# of the source files, if this is a multi-language target or has excluded files
			set (_flags "")
			cotire_determine_compiler_version("${_language}" COTIRE_${_language}_COMPILER)
			cotire_add_prefix_pch_inclusion_flags(
				"${_language}" "${CMAKE_${_language}_COMPILER_ID}" "${COTIRE_${_language}_COMPILER_VERSION}"
				"${_prefixFile}" "${_pchFile}" _flags)
			set_property (SOURCE ${_sourceFiles} APPEND_STRING PROPERTY COMPILE_FLAGS " ${_flags} ")
			# mark sources as cotired to prevent them from being used in another cotired target
			set_source_files_properties(${_sourceFiles} PROPERTIES COTIRE_TARGET "${_target}")
		endif()
		# make source files depend on precompiled header
		set_property (SOURCE ${_sourceFiles} APPEND PROPERTY OBJECT_DEPENDS "${_pchFile}")
	endif()
endfunction()

function (cotire_setup_prefix_file_inclusion _language _target _prefixFile)
	set (_sourceFiles ${ARGN})
	# force the inclusion of the prefix header for the given source files
	set (_flags "")
	cotire_determine_compiler_version("${_language}" COTIRE_${_language}_COMPILER)
	cotire_add_prefix_pch_inclusion_flags(
		"${_language}" "${CMAKE_${_language}_COMPILER_ID}" "${COTIRE_${_language}_COMPILER_VERSION}"
		"${_prefixFile}" "" _flags)
	set_property (SOURCE ${_sourceFiles} APPEND_STRING PROPERTY COMPILE_FLAGS " ${_flags} ")
	# mark sources as cotired to prevent them from being used in another cotired target
	set_source_files_properties(${_sourceFiles} PROPERTIES COTIRE_TARGET "${_target}")
	# make source files depend on prefix header
	set_property (SOURCE ${_sourceFiles} APPEND PROPERTY OBJECT_DEPENDS "${_prefixFile}")
endfunction()

function (cotire_get_first_set_property_value _propertyValueVar _type _object)
	set (_properties ${ARGN})
	foreach (_property ${_properties})
		get_property(_propertyValue ${_type} "${_object}" PROPERTY ${_property})
		if (_propertyValue)
			set (${_propertyValueVar} ${_propertyValue} PARENT_SCOPE)
			return()
		endif()
	endforeach()
	set (${_propertyValueVar} "" PARENT_SCOPE)
endfunction()

function (cotire_setup_combine_command _language _sourceDir _targetScript _joinedFile _cmdsVar)
	set (_files ${ARGN})
	set (_filesPaths "")
	foreach (_file ${_files})
		if (IS_ABSOLUTE "${_file}")
			set (_filePath "${_file}")
		else()
			get_filename_component(_filePath "${_sourceDir}/${_file}" ABSOLUTE)
		endif()
		file (RELATIVE_PATH _fileRelPath "${_sourceDir}" "${_filePath}")
		if (NOT IS_ABSOLUTE "${_fileRelPath}" AND NOT "${_fileRelPath}" MATCHES "^\\.\\.")
			list (APPEND _filesPaths "${_fileRelPath}")
		else()
			list (APPEND _filesPaths "${_filePath}")
		endif()
	endforeach()
	cotire_set_cmd_to_prologue(_prefixCmd)
	list (APPEND _prefixCmd -P "${COTIRE_CMAKE_MODULE_FILE}" "combine")
	if (_targetScript)
		list (APPEND _prefixCmd "${_targetScript}")
	endif()
	list (APPEND _prefixCmd "${_joinedFile}" ${_filesPaths})
	if (COTIRE_DEBUG)
		message (STATUS "add_custom_command: OUTPUT ${_joinedFile} COMMAND ${_prefixCmd} DEPENDS ${_files}")
	endif()
	set_property (SOURCE "${_joinedFile}" PROPERTY GENERATED TRUE)
	file (RELATIVE_PATH _joinedFileRelPath "${CMAKE_BINARY_DIR}" "${_joinedFile}")
	get_filename_component(_joinedFileName "${_joinedFileRelPath}" NAME_WE)
	if (_language AND _joinedFileName MATCHES "${COTIRE_UNITY_SOURCE_FILENAME_SUFFIX}$")
		set (_comment "Generating ${_language} unity source ${_joinedFileRelPath}")
	elseif (_language AND _joinedFileName MATCHES "${COTIRE_UNITY_SOURCE_FILENAME_SUFFIX}$")
		set (_comment "Generating ${_language} prefix header ${_joinedFileRelPath}")
	else()
		set (_comment "Generating ${_joinedFileRelPath}")
	endif()
	add_custom_command(
		OUTPUT "${_joinedFile}"
		COMMAND ${_prefixCmd}
		DEPENDS ${_files}
		COMMENT "${_comment}"
		WORKING_DIRECTORY "${_sourceDir}" VERBATIM)
	list (APPEND ${_cmdsVar} COMMAND ${_prefixCmd})
	set (${_cmdsVar} ${${_cmdsVar}} PARENT_SCOPE)
endfunction()

function (cotire_setup_target_pch_usage _languages _targetSourceDir _target _wholeTarget)
	if (XCODE)
		# for Xcode, we attach a pre-build action to generate the unity sources and prefix headers
		# if necessary, we also generate a single prefix header which includes all language specific prefix headers
		set (_prefixFiles "")
		foreach (_language ${_languages})
			get_property(_prefixFile TARGET ${_target} PROPERTY COTIRE_${_language}_PREFIX_HEADER)
			if (_prefixFile)
				list (APPEND _prefixFiles "${_prefixFile}")
			endif()
		endforeach()
		set (_cmds ${ARGN})
		list (LENGTH _prefixFiles _numberOfPrefixFiles)
		if (_numberOfPrefixFiles GREATER 1)
			cotire_make_prefix_file_path("" ${_target} _prefixHeader)
			cotire_setup_combine_command("" "${_targetSourceDir}" "" "${_prefixHeader}" _cmds ${_prefixFiles})
		else()
			set (_prefixHeader "${_prefixFiles}")
		endif()
		if (COTIRE_DEBUG)
			message (STATUS "add_custom_command: TARGET ${_target} PRE_BUILD ${_cmds}")
		endif()
		add_custom_command(TARGET "${_target}"
			PRE_BUILD ${_cmds}
			WORKING_DIRECTORY "${_targetSourceDir}"
			COMMENT "Updating target ${_target} prefix headers" VERBATIM)
		# make Xcode precompile the generated prefix header with ProcessPCH and ProcessPCH++
		set_target_properties(${_target} PROPERTIES XCODE_ATTRIBUTE_GCC_PRECOMPILE_PREFIX_HEADER "YES")
		set_target_properties(${_target} PROPERTIES XCODE_ATTRIBUTE_GCC_PREFIX_HEADER "${_prefixHeader}")
	elseif ("${CMAKE_GENERATOR}" MATCHES "Makefiles|Ninja")
		# for makefile based generator, we force inclusion of the prefix header for all target source files
		# if this is a single-language target without any excluded files
		if (_wholeTarget)
			set (_language "${_languages}")
			# for Visual Studio and Intel, precompiled header inclusion is always done on the source file level
			# see cotire_setup_pch_file_inclusion
			if (NOT CMAKE_${_language}_COMPILER_ID MATCHES "MSVC|Intel")
				get_property(_prefixFile TARGET ${_target} PROPERTY COTIRE_${_language}_PREFIX_HEADER)
				get_property(_pchFile TARGET ${_target} PROPERTY COTIRE_${_language}_PRECOMPILED_HEADER)
				set (_flags "")
				cotire_determine_compiler_version("${_language}" COTIRE_${_language}_COMPILER)
				cotire_add_prefix_pch_inclusion_flags(
					"${_language}" "${CMAKE_${_language}_COMPILER_ID}" "${COTIRE_${_language}_COMPILER_VERSION}"
					"${_prefixFile}" "${_pchFile}" _flags)
				set_property(TARGET ${_target} APPEND_STRING PROPERTY COMPILE_FLAGS " ${_flags} ")
			endif()
		endif()
	endif()
endfunction()

function (cotire_setup_unity_generation_commands _language _targetSourceDir _target _targetScript _unityFiles _cmdsVar)
	set (_dependencySources "")
	cotire_get_unity_source_dependencies(${_language} ${_target} _dependencySources ${ARGN})
	foreach (_unityFile ${_unityFiles})
		file (RELATIVE_PATH _unityFileRelPath "${CMAKE_BINARY_DIR}" "${_unityFile}")
		set_property (SOURCE "${_unityFile}" PROPERTY GENERATED TRUE)
		# set up compiled unity source dependencies
		# this ensures that missing source files are generated before the unity file is compiled
		if (COTIRE_DEBUG AND _dependencySources)
			message (STATUS "${_unityFile} OBJECT_DEPENDS ${_dependencySources}")
		endif()
		if (_dependencySources)
			set_property (SOURCE "${_unityFile}" PROPERTY OBJECT_DEPENDS ${_dependencySources})
		endif()
		if (WIN32 AND CMAKE_${_language}_COMPILER_ID MATCHES "MSVC|Intel")
			# unity file compilation results in potentially huge object file, thus use /bigobj by default unter MSVC and Windows Intel
			set_property (SOURCE "${_unityFile}" APPEND_STRING PROPERTY COMPILE_FLAGS "/bigobj")
		endif()
		cotire_set_cmd_to_prologue(_unityCmd)
		list (APPEND _unityCmd -P "${COTIRE_CMAKE_MODULE_FILE}" "unity" "${_targetScript}" "${_unityFile}")
		if (COTIRE_DEBUG)
			message (STATUS "add_custom_command: OUTPUT ${_unityFile} COMMAND ${_unityCmd} DEPENDS ${_targetScript}")
		endif()
		add_custom_command(
			OUTPUT "${_unityFile}"
			COMMAND ${_unityCmd}
			DEPENDS "${_targetScript}"
			COMMENT "Generating ${_language} unity source ${_unityFileRelPath}"
			WORKING_DIRECTORY "${_targetSourceDir}" VERBATIM)
		list (APPEND ${_cmdsVar} COMMAND ${_unityCmd})
	endforeach()
	list (LENGTH _unityFiles _numberOfUnityFiles)
	if (_numberOfUnityFiles GREATER 1)
		# create a joint unity file from all unity file segments
		cotire_make_single_unity_source_file_path(${_language} ${_target} _unityFile)
		cotire_setup_combine_command(${_language} "${_targetSourceDir}" "${_targetScript}" "${_unityFile}" ${_cmdsVar} ${_unityFiles})
	endif()
	set (${_cmdsVar} ${${_cmdsVar}} PARENT_SCOPE)
endfunction()

function (cotire_setup_single_prefix_generation_command _language _target _targetSourceDir _targetScript _prefixFile _unityFile _cmdsVar)
	set (_sourceFiles ${ARGN})
	set (_dependencySources "")
	cotire_get_prefix_header_dependencies(${_language} ${_target} _dependencySources ${_sourceFiles})
	cotire_set_cmd_to_prologue(_prefixCmd)
	list (APPEND _prefixCmd -P "${COTIRE_CMAKE_MODULE_FILE}" "prefix" "${_targetScript}" "${_prefixFile}" "${_unityFile}")
	set_property (SOURCE "${_prefixFile}" PROPERTY GENERATED TRUE)
	if (COTIRE_DEBUG)
		message (STATUS "add_custom_command: OUTPUT ${_prefixFile} COMMAND ${_prefixCmd} DEPENDS ${_targetScript} ${_unityFile} ${_dependencySources}")
	endif()
	file (RELATIVE_PATH _prefixFileRelPath "${CMAKE_BINARY_DIR}" "${_prefixFile}")
	add_custom_command(
		OUTPUT "${_prefixFile}" "${_prefixFile}.log"
		COMMAND ${_prefixCmd}
		DEPENDS "${_targetScript}" "${_unityFile}" ${_dependencySources}
		COMMENT "Generating ${_language} prefix header ${_prefixFileRelPath}"
		WORKING_DIRECTORY "${CMAKE_BINARY_DIR}" VERBATIM)
	list (APPEND ${_cmdsVar} COMMAND ${_prefixCmd})
	set (${_cmdsVar} ${${_cmdsVar}} PARENT_SCOPE)
endfunction()

function (cotire_setup_multi_prefix_generation_command _language _target _targetSourceDir _targetScript _prefixFile _unityFiles _cmdsVar)
	set (_sourceFiles ${ARGN})
	list (LENGTH _unityFiles _numberOfUnityFiles)
	if (_numberOfUnityFiles GREATER 1)
		cotire_make_single_unity_source_file_path(${_language} ${_target} _unityFile)
		cotire_setup_single_prefix_generation_command(
			${_language} ${_target} "${_targetSourceDir}" "${_targetScript}"
			"${_prefixFile}" "${_unityFile}" ${_cmdsVar} ${_sourceFiles})
	else()
		cotire_setup_single_prefix_generation_command(
			${_language} ${_target} "${_targetSourceDir}" "${_targetScript}"
			"${_prefixFile}" "${_unityFiles}" ${_cmdsVar} ${_sourceFiles})
	endif()
	set (${_cmdsVar} ${${_cmdsVar}} PARENT_SCOPE)
endfunction()

function (cotire_init_cotire_target_properties _target)
	get_property(_isSet TARGET ${_target} PROPERTY COTIRE_ENABLE_PRECOMPILED_HEADER SET)
	if (NOT _isSet)
		set_property(TARGET ${_target} PROPERTY COTIRE_ENABLE_PRECOMPILED_HEADER TRUE)
	endif()
	get_property(_isSet TARGET ${_target} PROPERTY COTIRE_ADD_UNITY_BUILD SET)
	if (NOT _isSet)
		set_property(TARGET ${_target} PROPERTY COTIRE_ADD_UNITY_BUILD TRUE)
	endif()
	get_property(_isSet TARGET ${_target} PROPERTY COTIRE_ADD_CLEAN SET)
	if (NOT _isSet)
		set_property(TARGET ${_target} PROPERTY COTIRE_ADD_CLEAN FALSE)
	endif()
	get_property(_isSet TARGET ${_target} PROPERTY COTIRE_PREFIX_HEADER_IGNORE_PATH SET)
	if (NOT _isSet)
		set_property(TARGET ${_target} PROPERTY COTIRE_PREFIX_HEADER_IGNORE_PATH "${CMAKE_SOURCE_DIR}")
		cotire_check_is_path_relative_to("${CMAKE_BINARY_DIR}" _isRelative "${CMAKE_SOURCE_DIR}")
		if (NOT _isRelative)
			set_property(TARGET ${_target} APPEND PROPERTY COTIRE_PREFIX_HEADER_IGNORE_PATH "${CMAKE_BINARY_DIR}")
		endif()
	endif()
	get_property(_isSet TARGET ${_target} PROPERTY COTIRE_PREFIX_HEADER_INCLUDE_PATH SET)
	if (NOT _isSet)
		set_property(TARGET ${_target} PROPERTY COTIRE_PREFIX_HEADER_INCLUDE_PATH "")
	endif()
	get_property(_isSet TARGET ${_target} PROPERTY COTIRE_UNITY_SOURCE_PRE_UNDEFS SET)
	if (NOT _isSet)
		set_property(TARGET ${_target} PROPERTY COTIRE_UNITY_SOURCE_PRE_UNDEFS "")
	endif()
	get_property(_isSet TARGET ${_target} PROPERTY COTIRE_UNITY_SOURCE_POST_UNDEFS SET)
	if (NOT _isSet)
		set_property(TARGET ${_target} PROPERTY COTIRE_UNITY_SOURCE_POST_UNDEFS "")
	endif()
	get_property(_isSet TARGET ${_target} PROPERTY COTIRE_UNITY_LINK_LIBRARIES_INIT SET)
	if (NOT _isSet)
		set_property(TARGET ${_target} PROPERTY COTIRE_UNITY_LINK_LIBRARIES_INIT "")
	endif()
	get_property(_isSet TARGET ${_target} PROPERTY COTIRE_UNITY_SOURCE_MAXIMUM_NUMBER_OF_INCLUDES SET)
	if (NOT _isSet)
		if (COTIRE_MAXIMUM_NUMBER_OF_UNITY_INCLUDES)
			set_property(TARGET ${_target} PROPERTY COTIRE_UNITY_SOURCE_MAXIMUM_NUMBER_OF_INCLUDES "${COTIRE_MAXIMUM_NUMBER_OF_UNITY_INCLUDES}")
		else()
			set_property(TARGET ${_target} PROPERTY COTIRE_UNITY_SOURCE_MAXIMUM_NUMBER_OF_INCLUDES "")
		endif()
	endif()
endfunction()

function (cotire_make_target_message _target _languages _disableMsg _targetMsgVar)
	get_target_property(_targetUsePCH ${_target} COTIRE_ENABLE_PRECOMPILED_HEADER)
	get_target_property(_targetAddSCU ${_target} COTIRE_ADD_UNITY_BUILD)
	string (REPLACE ";" " " _languagesStr "${_languages}")
	math (EXPR _numberOfExcludedFiles "${ARGC} - 4")
	if (_numberOfExcludedFiles EQUAL 0)
		set (_excludedStr "")
	elseif (COTIRE_VERBOSE OR _numberOfExcludedFiles LESS 4)
		string (REPLACE ";" ", " _excludedStr "excluding ${ARGN}")
	else()
		set (_excludedStr "excluding ${_numberOfExcludedFiles} files")
	endif()
	set (_targetMsg "")
	if (NOT _languages)
		set (_targetMsg "Target ${_target} cannot be cotired.")
		if (_disableMsg)
			set (_targetMsg "${_targetMsg} ${_disableMsg}")
		endif()
	elseif (NOT _targetUsePCH AND NOT _targetAddSCU)
		set (_targetMsg "${_languagesStr} target ${_target} cotired without unity build and precompiled header.")
		if (_disableMsg)
			set (_targetMsg "${_targetMsg} ${_disableMsg}")
		endif()
	elseif (NOT _targetUsePCH)
		if (_excludedStr)
			set (_targetMsg "${_languagesStr} target ${_target} cotired without precompiled header ${_excludedStr}.")
		else()
			set (_targetMsg "${_languagesStr} target ${_target} cotired without precompiled header.")
		endif()
		if (_disableMsg)
			set (_targetMsg "${_targetMsg} ${_disableMsg}")
		endif()
	elseif (NOT _targetAddSCU)
		if (_excludedStr)
			set (_targetMsg "${_languagesStr} target ${_target} cotired without unity build ${_excludedStr}.")
		else()
			set (_targetMsg "${_languagesStr} target ${_target} cotired without unity build.")
		endif()
	else()
		if (_excludedStr)
			set (_targetMsg "${_languagesStr} target ${_target} cotired ${_excludedStr}.")
		else()
			set (_targetMsg "${_languagesStr} target ${_target} cotired.")
		endif()
	endif()
	set (${_targetMsgVar} "${_targetMsg}" PARENT_SCOPE)
endfunction()

function (cotire_choose_target_languages _targetSourceDir _target _targetLanguagesVar)
	set (_languages ${ARGN})
	set (_allSourceFiles "")
	set (_allExcludedSourceFiles "")
	set (_allCotiredSourceFiles "")
	set (_targetLanguages "")
	get_target_property(_targetType ${_target} TYPE)
	get_target_property(_targetSourceFiles ${_target} SOURCES)
	get_target_property(_targetUsePCH ${_target} COTIRE_ENABLE_PRECOMPILED_HEADER)
	get_target_property(_targetAddSCU ${_target} COTIRE_ADD_UNITY_BUILD)
	set (_disableMsg "")
	foreach (_language ${_languages})
		get_target_property(_prefixHeader ${_target} COTIRE_${_language}_PREFIX_HEADER)
		get_target_property(_unityBuildFile ${_target} COTIRE_${_language}_UNITY_SOURCE)
		if (_prefixHeader OR _unityBuildFile)
			message (STATUS "Target ${_target} has already been cotired.")
			set (${_targetLanguagesVar} "" PARENT_SCOPE)
			return()
		endif()
		if (_targetUsePCH AND "${_language}" MATCHES "^C|CXX$")
			cotire_check_precompiled_header_support("${_language}" "${_targetSourceDir}" "${_target}" _disableMsg)
			if (_disableMsg)
				set (_targetUsePCH FALSE)
			endif()
		endif()
		set (_sourceFiles "")
		set (_excludedSources "")
		set (_cotiredSources "")
		cotire_filter_language_source_files(${_language} _sourceFiles _excludedSources _cotiredSources ${_targetSourceFiles})
		if (_sourceFiles OR _excludedSources OR _cotiredSources)
			list (APPEND _targetLanguages ${_language})
		endif()
		if (_sourceFiles)
			list (APPEND _allSourceFiles ${_sourceFiles})
		endif()
		if (_excludedSources)
			list (APPEND _allExcludedSourceFiles ${_excludedSources})
		endif()
		if (_cotiredSources)
			list (APPEND _allCotiredSourceFiles ${_cotiredSources})
		endif()
	endforeach()
	set (_targetMsgLevel STATUS)
	if (NOT _targetLanguages)
		string (REPLACE ";" " or " _languagesStr "${_languages}")
		set (_disableMsg "No ${_languagesStr} source files.")
		set (_targetUsePCH FALSE)
		set (_targetAddSCU FALSE)
	endif()
	if (_targetUsePCH)
		list (LENGTH _allSourceFiles _numberOfSources)
		if (_numberOfSources LESS ${COTIRE_MINIMUM_NUMBER_OF_TARGET_SOURCES})
			set (_disableMsg "Too few applicable sources.")
			set (_targetUsePCH FALSE)
		elseif (_allCotiredSourceFiles)
			cotire_get_source_file_property_values(_cotireTargets COTIRE_TARGET ${_allCotiredSourceFiles})
			list (REMOVE_DUPLICATES _cotireTargets)
			string (REPLACE ";" ", " _cotireTargetsStr "${_cotireTargets}")
			set (_disableMsg "Target sources already include a precompiled header for target(s) ${_cotireTargets}.")
			set (_disableMsg "${_disableMsg} Set target property COTIRE_ENABLE_PRECOMPILED_HEADER to FALSE for targets ${_target},")
			set (_disableMsg "${_disableMsg} ${_cotireTargetsStr} to get a workable build system.")
			set (_targetMsgLevel SEND_ERROR)
			set (_targetUsePCH FALSE)
		elseif (XCODE AND _allExcludedSourceFiles)
			# for Xcode, we cannot apply the precompiled header to individual sources, only to the whole target
			set (_disableMsg "Exclusion of source files not supported for generator Xcode.")
			set (_targetUsePCH FALSE)
		elseif (XCODE AND "${_targetType}" STREQUAL "OBJECT_LIBRARY")
			# for Xcode, we cannot apply the required PRE_BUILD action to generate the prefix header to an OBJECT_LIBRARY target
			set (_disableMsg "Required PRE_BUILD action not supported for OBJECT_LIBRARY targets for generator Xcode.")
			set (_targetUsePCH FALSE)
		endif()
	endif()
	set_property(TARGET ${_target} PROPERTY COTIRE_ENABLE_PRECOMPILED_HEADER ${_targetUsePCH})
	set_property(TARGET ${_target} PROPERTY COTIRE_ADD_UNITY_BUILD ${_targetAddSCU})
	cotire_make_target_message(${_target} "${_targetLanguages}" "${_disableMsg}" _targetMsg ${_allExcludedSourceFiles})
	if (_targetMsg)
		if (NOT DEFINED COTIREMSG_${_target})
			set (COTIREMSG_${_target} "")
		endif()
		if (COTIRE_VERBOSE OR NOT "${_targetMsgLevel}" STREQUAL "STATUS" OR
			NOT "${COTIREMSG_${_target}}" STREQUAL "${_targetMsg}")
			# cache message to avoid redundant messages on re-configure
			set (COTIREMSG_${_target} "${_targetMsg}" CACHE INTERNAL "${_target} cotire message.")
			message (${_targetMsgLevel} "${_targetMsg}")
		endif()
	endif()
	set (${_targetLanguagesVar} ${_targetLanguages} PARENT_SCOPE)
endfunction()

function (cotire_compute_unity_max_number_of_includes _target _maxIncludesVar)
	set (_sourceFiles ${ARGN})
	get_target_property(_maxIncludes ${_target} COTIRE_UNITY_SOURCE_MAXIMUM_NUMBER_OF_INCLUDES)
	if (_maxIncludes MATCHES "(-j|--parallel|--jobs) ?([0-9]*)")
		set (_numberOfThreads "${CMAKE_MATCH_2}")
		if (NOT _numberOfThreads)
			# use all available cores
			ProcessorCount(_numberOfThreads)
		endif()
		list (LENGTH _sourceFiles _numberOfSources)
		math (EXPR _maxIncludes "(${_numberOfSources} + ${_numberOfThreads} - 1) / ${_numberOfThreads}")
		# a unity source segment must not contain less than COTIRE_MINIMUM_NUMBER_OF_TARGET_SOURCES files
		if (_maxIncludes LESS ${COTIRE_MINIMUM_NUMBER_OF_TARGET_SOURCES})
			set (_maxIncludes ${COTIRE_MINIMUM_NUMBER_OF_TARGET_SOURCES})
		endif()
	elseif (NOT _maxIncludes MATCHES "[0-9]+")
		set (_maxIncludes 0)
	endif()
	if (COTIRE_DEBUG)
		message (STATUS "${_target} unity source max includes = ${_maxIncludes}")
	endif()
	set (${_maxIncludesVar} ${_maxIncludes} PARENT_SCOPE)
endfunction()

function (cotire_process_target_language _language _configurations _targetSourceDir _targetBinaryDir _target _wholeTargetVar _cmdsVar)
	set (${_cmdsVar} "" PARENT_SCOPE)
	get_target_property(_targetSourceFiles ${_target} SOURCES)
	set (_sourceFiles "")
	set (_excludedSources "")
	set (_cotiredSources "")
	cotire_filter_language_source_files(${_language} _sourceFiles _excludedSources _cotiredSources ${_targetSourceFiles})
	if (NOT _sourceFiles AND NOT _cotiredSources)
		return()
	endif()
	set (_wholeTarget ${${_wholeTargetVar}})
	set (_cmds "")
	# check for user provided unity source file list
	get_property(_unitySourceFiles TARGET ${_target} PROPERTY COTIRE_${_language}_UNITY_SOURCE_INIT)
	if (NOT _unitySourceFiles)
		set (_unitySourceFiles ${_sourceFiles} ${_cotiredSources})
	endif()
	cotire_generate_target_script(
		${_language} "${_configurations}" "${_targetSourceDir}" "${_targetBinaryDir}" ${_target} _targetScript ${_unitySourceFiles})
	cotire_compute_unity_max_number_of_includes(${_target} _maxIncludes ${_unitySourceFiles})
	cotire_make_unity_source_file_paths(${_language} ${_target} ${_maxIncludes} _unityFiles ${_unitySourceFiles})
	if (NOT _unityFiles)
		return()
	endif()
	cotire_setup_unity_generation_commands(
		${_language} "${_targetSourceDir}" ${_target} "${_targetScript}" "${_unityFiles}" _cmds ${_unitySourceFiles})
	cotire_make_prefix_file_path(${_language} ${_target} _prefixFile)
	if (_prefixFile)
		# check for user provided prefix header files
		get_property(_prefixHeaderFiles TARGET ${_target} PROPERTY COTIRE_${_language}_PREFIX_HEADER_INIT)
		if (_prefixHeaderFiles)
			cotire_setup_combine_command(${_language} "${_targetSourceDir}" "${_targetScript}" "${_prefixFile}" _cmds ${_prefixHeaderFiles})
		else()
			cotire_setup_multi_prefix_generation_command(
				${_language} ${_target} "${_targetSourceDir}" "${_targetScript}" "${_prefixFile}" "${_unityFiles}" _cmds ${_unitySourceFiles})
		endif()
		get_target_property(_targetUsePCH ${_target} COTIRE_ENABLE_PRECOMPILED_HEADER)
		if (_targetUsePCH)
			cotire_make_pch_file_path(${_language} "${_targetSourceDir}" ${_target} _pchFile)
			if (_pchFile)
				cotire_setup_pch_file_compilation(
					${_language} ${_target} "${_targetSourceDir}" "${_targetScript}" "${_prefixFile}" "${_pchFile}" ${_sourceFiles})
				if (_excludedSources)
					set (_wholeTarget FALSE)
				endif()
				cotire_setup_pch_file_inclusion(
					${_language} ${_target} ${_wholeTarget} "${_prefixFile}" "${_pchFile}" ${_sourceFiles})
			endif()
		elseif (_prefixHeaderFiles)
			# user provided prefix header must be included
			cotire_setup_prefix_file_inclusion(
				${_language} ${_target} "${_prefixFile}" ${_sourceFiles})
		endif()
	endif()
	# mark target as cotired for language
	set_property(TARGET ${_target} PROPERTY COTIRE_${_language}_UNITY_SOURCE "${_unityFiles}")
	if (_prefixFile)
		set_property(TARGET ${_target} PROPERTY COTIRE_${_language}_PREFIX_HEADER "${_prefixFile}")
		if (_targetUsePCH AND _pchFile)
			set_property(TARGET ${_target} PROPERTY COTIRE_${_language}_PRECOMPILED_HEADER "${_pchFile}")
		endif()
	endif()
	set (${_wholeTargetVar} ${_wholeTarget} PARENT_SCOPE)
	set (${_cmdsVar} ${_cmds} PARENT_SCOPE)
endfunction()

function (cotire_setup_clean_target _target)
	set (_cleanTargetName "${_target}${COTIRE_CLEAN_TARGET_SUFFIX}")
	if (NOT TARGET "${_cleanTargetName}")
		cotire_set_cmd_to_prologue(_cmds)
		get_filename_component(_outputDir "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}" ABSOLUTE)
		list (APPEND _cmds -P "${COTIRE_CMAKE_MODULE_FILE}" "cleanup" "${_outputDir}" "${COTIRE_INTDIR}" "${_target}")
		add_custom_target(${_cleanTargetName} COMMAND ${_cmds} WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
			COMMENT "Cleaning up target ${_target} cotire generated files" VERBATIM)
		cotire_init_target("${_cleanTargetName}")
	endif()
endfunction()

function (cotire_setup_pch_target _languages _configurations _target)
	if ("${CMAKE_GENERATOR}" MATCHES "Makefiles|Ninja")
		# for makefile based generators, we add a custom target to trigger the generation of the cotire related files
		set (_dependsFiles "")
		foreach (_language ${_languages})
			set (_props COTIRE_${_language}_PREFIX_HEADER COTIRE_${_language}_UNITY_SOURCE)
			if (NOT CMAKE_${_language}_COMPILER_ID MATCHES "MSVC|Intel")
				# Visual Studio and Intel only create precompiled header as a side effect
				list (INSERT _props 0 COTIRE_${_language}_PRECOMPILED_HEADER)
			endif()
			cotire_get_first_set_property_value(_dependsFile TARGET ${_target} ${_props})
			if (_dependsFile)
				list (APPEND _dependsFiles "${_dependsFile}")
			endif()
		endforeach()
		if (_dependsFiles)
			set (_pchTargetName "${_target}${COTIRE_PCH_TARGET_SUFFIX}")
			add_custom_target("${_pchTargetName}" DEPENDS ${_dependsFiles})
			cotire_init_target("${_pchTargetName}")
			cotire_add_to_pch_all_target(${_pchTargetName})
		endif()
	else()
		# for other generators, we add the "clean all" target to clean up the precompiled header
		cotire_setup_clean_all_target()
	endif()
endfunction()

function (cotire_setup_unity_build_target _languages _configurations _targetSourceDir _target)
	get_target_property(_unityTargetName ${_target} COTIRE_UNITY_TARGET_NAME)
	if (NOT _unityTargetName)
		set (_unityTargetName "${_target}${COTIRE_UNITY_BUILD_TARGET_SUFFIX}")
	endif()
	# determine unity target sub type
	get_target_property(_targetType ${_target} TYPE)
	if ("${_targetType}" STREQUAL "EXECUTABLE")
		set (_unityTargetSubType "")
	elseif (_targetType MATCHES "(STATIC|SHARED|MODULE|OBJECT)_LIBRARY")
		set (_unityTargetSubType "${CMAKE_MATCH_1}")
	else()
		message (WARNING "Unknown target type ${_targetType}.")
		return()
	endif()
	# determine unity target sources
	get_target_property(_targetSourceFiles ${_target} SOURCES)
	set (_unityTargetSources ${_targetSourceFiles})
	foreach (_language ${_languages})
		get_property(_unityFiles TARGET ${_target} PROPERTY COTIRE_${_language}_UNITY_SOURCE)
		if (_unityFiles)
			# remove source files that are included in the unity source
			set (_sourceFiles "")
			set (_excludedSources "")
			set (_cotiredSources "")
			cotire_filter_language_source_files(${_language} _sourceFiles _excludedSources _cotiredSources ${_targetSourceFiles})
			if (_sourceFiles OR _cotiredSources)
				list (REMOVE_ITEM _unityTargetSources ${_sourceFiles} ${_cotiredSources})
			endif()
			# if cotire is applied to a target which has not been added in the current source dir,
			# non-existing files cannot be referenced from the unity build target (this is a CMake restriction)
			if (NOT "${_targetSourceDir}" STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}")
				set (_nonExistingFiles "")
				foreach (_file ${_unityTargetSources})
					if (NOT EXISTS "${_file}")
						list (APPEND _nonExistingFiles "${_file}")
					endif()
				endforeach()
				if (_nonExistingFiles)
					if (COTIRE_VERBOSE)
						message (STATUS "removing non-existing ${_nonExistingFiles} from ${_unityTargetName}")
					endif()
					list (REMOVE_ITEM _unityTargetSources ${_nonExistingFiles})
				endif()
			endif()
			# add unity source files instead
			list (APPEND _unityTargetSources ${_unityFiles})
		endif()
	endforeach()
	if (COTIRE_DEBUG)
		message (STATUS "add ${_targetType} ${_unityTargetName} ${_unityTargetSubType} EXCLUDE_FROM_ALL ${_unityTargetSources}")
	endif()
	# generate unity target
	if ("${_targetType}" STREQUAL "EXECUTABLE")
		add_executable(${_unityTargetName} ${_unityTargetSubType} EXCLUDE_FROM_ALL ${_unityTargetSources})
	else()
		add_library(${_unityTargetName} ${_unityTargetSubType} EXCLUDE_FROM_ALL ${_unityTargetSources})
	endif()
	set (_outputDirProperties
		ARCHIVE_OUTPUT_DIRECTORY ARCHIVE_OUTPUT_DIRECTORY_<CONFIG>
		LIBRARY_OUTPUT_DIRECTORY LIBRARY_OUTPUT_DIRECTORY_<CONFIG>
		RUNTIME_OUTPUT_DIRECTORY RUNTIME_OUTPUT_DIRECTORY_<CONFIG>)
	# copy output location properties
	if (COTIRE_UNITY_OUTPUT_DIRECTORY)
		set (_setDefaultOutputDir TRUE)
		if (IS_ABSOLUTE "${COTIRE_UNITY_OUTPUT_DIRECTORY}")
			set (_outputDir "${COTIRE_UNITY_OUTPUT_DIRECTORY}")
		else()
			cotire_copy_set_properites("${_configurations}" TARGET ${_target} ${_unityTargetName} ${_outputDirProperties})
			cotire_resolve_config_properites("${_configurations}" _properties ${_outputDirProperties})
			foreach (_property ${_properties})
				get_property(_outputDir TARGET ${_target} PROPERTY ${_property})
				if (_outputDir)
					get_filename_component(_outputDir "${_outputDir}/${COTIRE_UNITY_OUTPUT_DIRECTORY}" ABSOLUTE)
					set_property(TARGET ${_unityTargetName} PROPERTY ${_property} "${_outputDir}")
					set (_setDefaultOutputDir FALSE)
				endif()
			endforeach()
			if (_setDefaultOutputDir)
				get_filename_component(_outputDir "${CMAKE_CURRENT_BINARY_DIR}/${COTIRE_UNITY_OUTPUT_DIRECTORY}" ABSOLUTE)
			endif()
		endif()
		if (_setDefaultOutputDir)
			set_target_properties(${_unityTargetName} PROPERTIES
				ARCHIVE_OUTPUT_DIRECTORY "${_outputDir}"
				LIBRARY_OUTPUT_DIRECTORY "${_outputDir}"
				RUNTIME_OUTPUT_DIRECTORY "${_outputDir}")
		endif()
	else()
		cotire_copy_set_properites("${_configurations}" TARGET ${_target} ${_unityTargetName} ${_outputDirProperties})
	endif()
	# copy output name
	cotire_copy_set_properites("${_configurations}" TARGET ${_target} ${_unityTargetName}
		ARCHIVE_OUTPUT_NAME ARCHIVE_OUTPUT_NAME_<CONFIG>
		LIBRARY_OUTPUT_NAME LIBRARY_OUTPUT_NAME_<CONFIG>
		OUTPUT_NAME OUTPUT_NAME_<CONFIG>
		RUNTIME_OUTPUT_NAME RUNTIME_OUTPUT_NAME_<CONFIG>
		PREFIX <CONFIG>_POSTFIX SUFFIX)
	# copy compile stuff
	cotire_copy_set_properites("${_configurations}" TARGET ${_target} ${_unityTargetName}
		COMPILE_DEFINITIONS COMPILE_DEFINITIONS_<CONFIG>
		COMPILE_FLAGS COMPILE_OPTIONS
		Fortran_FORMAT Fortran_MODULE_DIRECTORY
		INCLUDE_DIRECTORIES
		INTERPROCEDURAL_OPTIMIZATION INTERPROCEDURAL_OPTIMIZATION_<CONFIG>
		POSITION_INDEPENDENT_CODE
		C_VISIBILITY_PRESET CXX_VISIBILITY_PRESET VISIBILITY_INLINES_HIDDEN)
	# copy interface stuff
	cotire_copy_set_properites("${_configurations}" TARGET ${_target} ${_unityTargetName}
		COMPATIBLE_INTERFACE_BOOL COMPATIBLE_INTERFACE_STRING
		INTERFACE_COMPILE_DEFINITIONS INTERFACE_COMPILE_OPTIONS INTERFACE_INCLUDE_DIRECTORIES
		INTERFACE_LINK_LIBRARIES INTERFACE_POSITION_INDEPENDENT_CODE INTERFACE_SYSTEM_INCLUDE_DIRECTORIES)
	# copy link stuff
	cotire_copy_set_properites("${_configurations}" TARGET ${_target} ${_unityTargetName}
		BUILD_WITH_INSTALL_RPATH INSTALL_RPATH INSTALL_RPATH_USE_LINK_PATH SKIP_BUILD_RPATH
		LINKER_LANGUAGE LINK_DEPENDS LINK_DEPENDS_NO_SHARED
		LINK_FLAGS LINK_FLAGS_<CONFIG>
		LINK_INTERFACE_LIBRARIES LINK_INTERFACE_LIBRARIES_<CONFIG>
		LINK_INTERFACE_MULTIPLICITY LINK_INTERFACE_MULTIPLICITY_<CONFIG>
		LINK_SEARCH_START_STATIC LINK_SEARCH_END_STATIC
		STATIC_LIBRARY_FLAGS STATIC_LIBRARY_FLAGS_<CONFIG>
		NO_SONAME SOVERSION VERSION)
	# copy Qt stuff
	cotire_copy_set_properites("${_configurations}" TARGET ${_target} ${_unityTargetName}
		AUTOMOC AUTOMOC_MOC_OPTIONS)
	# copy cmake stuff
	cotire_copy_set_properites("${_configurations}" TARGET ${_target} ${_unityTargetName}
		IMPLICIT_DEPENDS_INCLUDE_TRANSFORM RULE_LAUNCH_COMPILE RULE_LAUNCH_CUSTOM RULE_LAUNCH_LINK)
	# copy Apple platform specific stuff
	cotire_copy_set_properites("${_configurations}" TARGET ${_target} ${_unityTargetName}
		BUNDLE BUNDLE_EXTENSION FRAMEWORK INSTALL_NAME_DIR MACOSX_BUNDLE MACOSX_BUNDLE_INFO_PLIST MACOSX_FRAMEWORK_INFO_PLIST
		MACOSX_RPATH OSX_ARCHITECTURES OSX_ARCHITECTURES_<CONFIG> PRIVATE_HEADER PUBLIC_HEADER RESOURCE)
	# copy Windows platform specific stuff
	cotire_copy_set_properites("${_configurations}" TARGET ${_target} ${_unityTargetName}
		GNUtoMS
		PDB_NAME PDB_NAME_<CONFIG> PDB_OUTPUT_DIRECTORY PDB_OUTPUT_DIRECTORY_<CONFIG>
		VS_DOTNET_REFERENCES VS_GLOBAL_KEYWORD VS_GLOBAL_PROJECT_TYPES VS_GLOBAL_ROOTNAMESPACE VS_KEYWORD
		VS_SCC_AUXPATH VS_SCC_LOCALPATH VS_SCC_PROJECTNAME VS_SCC_PROVIDER
		VS_WINRT_EXTENSIONS VS_WINRT_REFERENCES WIN32_EXECUTABLE)
	# use output name from original target
	get_target_property(_targetOutputName ${_unityTargetName} OUTPUT_NAME)
	if (NOT _targetOutputName)
		set_property(TARGET ${_unityTargetName} PROPERTY OUTPUT_NAME "${_target}")
	endif()
	# use export symbol from original target
	cotire_get_target_export_symbol("${_target}" _defineSymbol)
	if (_defineSymbol)
		set_property(TARGET ${_unityTargetName} PROPERTY DEFINE_SYMBOL "${_defineSymbol}")
		if ("${_targetType}" STREQUAL "EXECUTABLE")
			set_property(TARGET ${_unityTargetName} PROPERTY ENABLE_EXPORTS TRUE)
		endif()
	endif()
	cotire_init_target(${_unityTargetName})
	cotire_add_to_unity_all_target(${_unityTargetName})
	set_property(TARGET ${_target} PROPERTY COTIRE_UNITY_TARGET_NAME "${_unityTargetName}")
endfunction(cotire_setup_unity_build_target)

function (cotire_target _target)
	set(_options "")
	set(_oneValueArgs SOURCE_DIR BINARY_DIR)
	set(_multiValueArgs LANGUAGES CONFIGURATIONS)
	cmake_parse_arguments(_option "${_options}" "${_oneValueArgs}" "${_multiValueArgs}" ${ARGN})
	if (NOT _option_SOURCE_DIR)
		set (_option_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
	endif()
	if (NOT _option_BINARY_DIR)
		set (_option_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}")
	endif()
	if (NOT _option_LANGUAGES)
		get_property (_option_LANGUAGES GLOBAL PROPERTY ENABLED_LANGUAGES)
	endif()
	if (NOT _option_CONFIGURATIONS)
		if (CMAKE_CONFIGURATION_TYPES)
			set (_option_CONFIGURATIONS ${CMAKE_CONFIGURATION_TYPES})
		elseif (CMAKE_BUILD_TYPE)
			set (_option_CONFIGURATIONS "${CMAKE_BUILD_TYPE}")
		else()
			set (_option_CONFIGURATIONS "None")
		endif()
	endif()
	# trivial checks
	get_target_property(_imported ${_target} IMPORTED)
	if (_imported)
		message (WARNING "Imported target ${_target} cannot be cotired.")
		return()
	endif()
	# resolve alias
	get_target_property(_aliasName ${_target} ALIASED_TARGET)
	if (_aliasName)
		if (COTIRE_DEBUG)
			message (STATUS "${_target} is an alias. Applying cotire to aliased target ${_aliasName} instead.")
		endif()
		set (_target ${_aliasName})
	endif()
	# check if target needs to be cotired for build type
	# when using configuration types, the test is performed at build time
	cotire_init_cotire_target_properties(${_target})
	if (NOT CMAKE_CONFIGURATION_TYPES)
		if (CMAKE_BUILD_TYPE)
			list (FIND _option_CONFIGURATIONS "${CMAKE_BUILD_TYPE}" _index)
		else()
			list (FIND _option_CONFIGURATIONS "None" _index)
		endif()
		if (_index EQUAL -1)
			if (COTIRE_DEBUG)
				message (STATUS "CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} not cotired (${_option_CONFIGURATIONS})")
			endif()
			return()
		endif()
	endif()
	# choose languages that apply to the target
	cotire_choose_target_languages("${_option_SOURCE_DIR}" "${_target}" _targetLanguages ${_option_LANGUAGES})
	if (NOT _targetLanguages)
		return()
	endif()
	list (LENGTH _targetLanguages _numberOfLanguages)
	if (_numberOfLanguages GREATER 1)
		set (_wholeTarget FALSE)
	else()
		set (_wholeTarget TRUE)
	endif()
	set (_cmds "")
	foreach (_language ${_targetLanguages})
		cotire_process_target_language("${_language}" "${_option_CONFIGURATIONS}"
			"${_option_SOURCE_DIR}" "${_option_BINARY_DIR}" ${_target} _wholeTarget _cmd)
		if (_cmd)
			list (APPEND _cmds ${_cmd})
		endif()
	endforeach()
	get_target_property(_targetAddSCU ${_target} COTIRE_ADD_UNITY_BUILD)
	if (_targetAddSCU)
		cotire_setup_unity_build_target("${_targetLanguages}" "${_option_CONFIGURATIONS}" "${_option_SOURCE_DIR}" ${_target})
	endif()
	get_target_property(_targetUsePCH ${_target} COTIRE_ENABLE_PRECOMPILED_HEADER)
	if (_targetUsePCH)
		cotire_setup_target_pch_usage("${_targetLanguages}" "${_option_SOURCE_DIR}" ${_target} ${_wholeTarget} ${_cmds})
		cotire_setup_pch_target("${_targetLanguages}" "${_option_CONFIGURATIONS}" ${_target})
	endif()
	get_target_property(_targetAddCleanTarget ${_target} COTIRE_ADD_CLEAN)
	if (_targetAddCleanTarget)
		cotire_setup_clean_target(${_target})
	endif()
endfunction(cotire_target)

function(cotire_target_link_libraries _target)
	get_target_property(_unityTargetName ${_target} COTIRE_UNITY_TARGET_NAME)
	if (TARGET "${_unityTargetName}")
		get_target_property(_linkLibrariesStrategy ${_target} COTIRE_UNITY_LINK_LIBRARIES_INIT)
		if (COTIRE_DEBUG)
			message (STATUS "unity target ${_unityTargetName} link strategy: ${_linkLibrariesStrategy}")
		endif()
		if ("${_linkLibrariesStrategy}" MATCHES "^(COPY|COPY_UNITY)$")
			if (CMAKE_VERSION VERSION_LESS "2.8.11")
				message (WARNING "Unity target link strategy ${_linkLibrariesStrategy} requires CMake 2.8.11 or later. Defaulting to NONE for ${_target}.")
				return()
			endif()
			get_target_property(_linkLibraries ${_target} LINK_LIBRARIES)
			if (_linkLibraries)
				if (COTIRE_DEBUG)
					message (STATUS "target ${_target} link libraries: ${_linkLibraries}")
				endif()
				set (_unityTargetLibraries "")
				foreach (_library ${_linkLibraries})
					if (TARGET "${_library}" AND "${_linkLibrariesStrategy}" MATCHES "COPY_UNITY")
						get_target_property(_libraryUnityTargetName ${_library} COTIRE_UNITY_TARGET_NAME)
						if (TARGET "${_libraryUnityTargetName}")
							list (APPEND _unityTargetLibraries "${_libraryUnityTargetName}")
						else()
							list (APPEND _unityTargetLibraries "${_library}")
						endif()
					else()
						list (APPEND _unityTargetLibraries "${_library}")
					endif()
				endforeach()
				set_property(TARGET ${_unityTargetName} APPEND PROPERTY LINK_LIBRARIES ${_unityTargetLibraries})
				if (COTIRE_DEBUG)
					message (STATUS "set unity target ${_unityTargetName} link libraries: ${_unityTargetLibraries}")
				endif()
			endif()
		endif()
	endif()
endfunction(cotire_target_link_libraries)

function (cotire_cleanup _binaryDir _cotireIntermediateDirName _targetName)
	if (_targetName)
		file (GLOB_RECURSE _cotireFiles "${_binaryDir}/${_targetName}*.*")
	else()
		file (GLOB_RECURSE _cotireFiles "${_binaryDir}/*.*")
	endif()
	# filter files in intermediate directory
	set (_filesToRemove "")
	foreach (_file ${_cotireFiles})
		get_filename_component(_dir "${_file}" PATH)
		get_filename_component(_dirName "${_dir}" NAME)
		if ("${_dirName}" STREQUAL "${_cotireIntermediateDirName}")
			list (APPEND _filesToRemove "${_file}")
		endif()
	endforeach()
	if (_filesToRemove)
		if (COTIRE_VERBOSE)
			message (STATUS "removing ${_filesToRemove}")
		endif()
		file (REMOVE ${_filesToRemove})
	endif()
endfunction()

function (cotire_init_target _targetName)
	if (COTIRE_TARGETS_FOLDER)
		set_target_properties(${_targetName} PROPERTIES FOLDER "${COTIRE_TARGETS_FOLDER}")
	endif()
	if (MSVC_IDE)
		set_target_properties(${_targetName} PROPERTIES EXCLUDE_FROM_DEFAULT_BUILD TRUE)
	endif()
endfunction()

function (cotire_add_to_pch_all_target _pchTargetName)
	set (_targetName "${COTIRE_PCH_ALL_TARGET_NAME}")
	if (NOT TARGET "${_targetName}")
		add_custom_target("${_targetName}" WORKING_DIRECTORY "${CMAKE_BINARY_DIR}" VERBATIM)
		cotire_init_target("${_targetName}")
	endif()
	cotire_setup_clean_all_target()
	add_dependencies(${_targetName} ${_pchTargetName})
endfunction()

function (cotire_add_to_unity_all_target _unityTargetName)
	set (_targetName "${COTIRE_UNITY_BUILD_ALL_TARGET_NAME}")
	if (NOT TARGET "${_targetName}")
		add_custom_target("${_targetName}" WORKING_DIRECTORY "${CMAKE_BINARY_DIR}" VERBATIM)
		cotire_init_target("${_targetName}")
	endif()
	cotire_setup_clean_all_target()
	add_dependencies(${_targetName} ${_unityTargetName})
endfunction()

function (cotire_setup_clean_all_target)
	set (_targetName "${COTIRE_CLEAN_ALL_TARGET_NAME}")
	if (NOT TARGET "${_targetName}")
		cotire_set_cmd_to_prologue(_cmds)
		list (APPEND _cmds -P "${COTIRE_CMAKE_MODULE_FILE}" "cleanup" "${CMAKE_BINARY_DIR}" "${COTIRE_INTDIR}")
		add_custom_target(${_targetName} COMMAND ${_cmds}
			WORKING_DIRECTORY "${CMAKE_BINARY_DIR}" COMMENT "Cleaning up all cotire generated files" VERBATIM)
		cotire_init_target("${_targetName}")
	endif()
endfunction()

function (cotire)
	set(_options "")
	set(_oneValueArgs SOURCE_DIR BINARY_DIR)
	set(_multiValueArgs LANGUAGES CONFIGURATIONS)
	cmake_parse_arguments(_option "${_options}" "${_oneValueArgs}" "${_multiValueArgs}" ${ARGN})
	set (_targets ${_option_UNPARSED_ARGUMENTS})
	if (NOT _option_SOURCE_DIR)
		set (_option_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
	endif()
	if (NOT _option_BINARY_DIR)
		set (_option_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}")
	endif()
	foreach (_target ${_targets})
		if (TARGET ${_target})
			cotire_target(${_target} LANGUAGES ${_option_LANGUAGES} CONFIGURATIONS ${_option_CONFIGURATIONS}
				SOURCE_DIR "${_option_SOURCE_DIR}" BINARY_DIR "${_option_BINARY_DIR}")
		else()
			message (WARNING "${_target} is not a target.")
		endif()
	endforeach()
	foreach (_target ${_targets})
		if (TARGET ${_target})
			cotire_target_link_libraries(${_target})
		endif()
	endforeach()
endfunction()

if (CMAKE_SCRIPT_MODE_FILE)

	# cotire is being run in script mode
	# locate -P on command args
	set (COTIRE_ARGC -1)
	foreach (_index RANGE ${CMAKE_ARGC})
		if (COTIRE_ARGC GREATER -1)
			set (COTIRE_ARGV${COTIRE_ARGC} "${CMAKE_ARGV${_index}}")
			math (EXPR COTIRE_ARGC "${COTIRE_ARGC} + 1")
		elseif ("${CMAKE_ARGV${_index}}" STREQUAL "-P")
			set (COTIRE_ARGC 0)
		endif()
	endforeach()

	# include target script if available
	if ("${COTIRE_ARGV2}" MATCHES "\\.cmake$")
		# the included target scripts sets up additional variables relating to the target (e.g., COTIRE_TARGET_SOURCES)
		include("${COTIRE_ARGV2}")
	endif()

	if (COTIRE_DEBUG)
		message (STATUS "${COTIRE_ARGV0} ${COTIRE_ARGV1} ${COTIRE_ARGV2} ${COTIRE_ARGV3} ${COTIRE_ARGV4} ${COTIRE_ARGV5}")
	endif()

	if (WIN32)
		# for MSVC, compiler IDs may not always be set correctly
		if (MSVC)
			set (CMAKE_C_COMPILER_ID "MSVC")
			set (CMAKE_CXX_COMPILER_ID "MSVC")
		endif()
	endif()

	if (NOT COTIRE_BUILD_TYPE)
		set (COTIRE_BUILD_TYPE "None")
	endif()
	string (TOUPPER "${COTIRE_BUILD_TYPE}" _upperConfig)
	set (_includeDirs ${COTIRE_TARGET_INCLUDE_DIRECTORIES_${_upperConfig}})
	set (_compileDefinitions ${COTIRE_TARGET_COMPILE_DEFINITIONS_${_upperConfig}})
	set (_compileFlags ${COTIRE_TARGET_COMPILE_FLAGS_${_upperConfig}})
	# check if target has been cotired for actual build type COTIRE_BUILD_TYPE
	list (FIND COTIRE_TARGET_CONFIGURATION_TYPES "${COTIRE_BUILD_TYPE}" _index)
	if (_index GREATER -1)
		set (_sources ${COTIRE_TARGET_SOURCES})
		set (_sourcesDefinitions ${COTIRE_TARGET_SOURCES_COMPILE_DEFINITIONS_${_upperConfig}})
	else()
		if (COTIRE_DEBUG)
			message (STATUS "COTIRE_BUILD_TYPE=${COTIRE_BUILD_TYPE} not cotired (${COTIRE_TARGET_CONFIGURATION_TYPES})")
		endif()
		set (_sources "")
		set (_sourcesDefinitions "")
	endif()
	set (_targetPreUndefs ${COTIRE_TARGET_PRE_UNDEFS})
	set (_targetPostUndefs ${COTIRE_TARGET_POST_UNDEFS})
	set (_sourcesPreUndefs ${COTIRE_TARGET_SOURCES_PRE_UNDEFS})
	set (_sourcesPostUndefs ${COTIRE_TARGET_SOURCES_POST_UNDEFS})

	if ("${COTIRE_ARGV1}" STREQUAL "unity")

		cotire_select_unity_source_files("${COTIRE_ARGV3}" _sources ${_sources})
		cotire_generate_unity_source(
			"${COTIRE_ARGV3}" ${_sources}
			LANGUAGE "${COTIRE_TARGET_LANGUAGE}"
			DEPENDS "${COTIRE_ARGV0}" "${COTIRE_ARGV2}"
			SOURCES_COMPILE_DEFINITIONS ${_sourcesDefinitions}
			PRE_UNDEFS ${_targetPreUndefs}
			POST_UNDEFS ${_targetPostUndefs}
			SOURCES_PRE_UNDEFS ${_sourcesPreUndefs}
			SOURCES_POST_UNDEFS ${_sourcesPostUndefs})

	elseif ("${COTIRE_ARGV1}" STREQUAL "prefix")

		set (_files "")
		foreach (_index RANGE 4 ${COTIRE_ARGC})
			if (COTIRE_ARGV${_index})
				list (APPEND _files "${COTIRE_ARGV${_index}}")
			endif()
		endforeach()

		cotire_generate_prefix_header(
			"${COTIRE_ARGV3}" ${_files}
			COMPILER_EXECUTABLE "${CMAKE_${COTIRE_TARGET_LANGUAGE}_COMPILER}"
			COMPILER_ARG1 ${CMAKE_${COTIRE_TARGET_LANGUAGE}_COMPILER_ARG1}
			COMPILER_ID "${CMAKE_${COTIRE_TARGET_LANGUAGE}_COMPILER_ID}"
			COMPILER_VERSION "${COTIRE_${COTIRE_TARGET_LANGUAGE}_COMPILER_VERSION}"
			LANGUAGE "${COTIRE_TARGET_LANGUAGE}"
			DEPENDS "${COTIRE_ARGV0}" "${COTIRE_ARGV4}" ${COTIRE_TARGET_PREFIX_DEPENDS}
			IGNORE_PATH "${COTIRE_TARGET_IGNORE_PATH};${COTIRE_ADDITIONAL_PREFIX_HEADER_IGNORE_PATH}"
			INCLUDE_PATH ${COTIRE_TARGET_INCLUDE_PATH}
			IGNORE_EXTENSIONS "${CMAKE_${COTIRE_TARGET_LANGUAGE}_SOURCE_FILE_EXTENSIONS};${COTIRE_ADDITIONAL_PREFIX_HEADER_IGNORE_EXTENSIONS}"
			INCLUDE_DIRECTORIES ${_includeDirs}
			COMPILE_DEFINITIONS ${_compileDefinitions}
			COMPILE_FLAGS ${_compileFlags})

	elseif ("${COTIRE_ARGV1}" STREQUAL "precompile")

		set (_files "")
		foreach (_index RANGE 5 ${COTIRE_ARGC})
			if (COTIRE_ARGV${_index})
				list (APPEND _files "${COTIRE_ARGV${_index}}")
			endif()
		endforeach()

		cotire_precompile_prefix_header(
			"${COTIRE_ARGV3}" "${COTIRE_ARGV4}" "${COTIRE_ARGV5}"
			COMPILER_EXECUTABLE "${CMAKE_${COTIRE_TARGET_LANGUAGE}_COMPILER}"
			COMPILER_ARG1 ${CMAKE_${COTIRE_TARGET_LANGUAGE}_COMPILER_ARG1}
			COMPILER_ID "${CMAKE_${COTIRE_TARGET_LANGUAGE}_COMPILER_ID}"
			COMPILER_VERSION "${COTIRE_${COTIRE_TARGET_LANGUAGE}_COMPILER_VERSION}"
			LANGUAGE "${COTIRE_TARGET_LANGUAGE}"
			INCLUDE_DIRECTORIES ${_includeDirs}
			COMPILE_DEFINITIONS ${_compileDefinitions}
			COMPILE_FLAGS ${_compileFlags})

	elseif ("${COTIRE_ARGV1}" STREQUAL "combine")

		if (COTIRE_TARGET_LANGUAGE)
			set (_startIndex 3)
		else()
			set (_startIndex 2)
		endif()
		set (_files "")
		foreach (_index RANGE ${_startIndex} ${COTIRE_ARGC})
			if (COTIRE_ARGV${_index})
				list (APPEND _files "${COTIRE_ARGV${_index}}")
			endif()
		endforeach()
		if (COTIRE_TARGET_LANGUAGE)
			cotire_generate_unity_source(${_files} LANGUAGE "${COTIRE_TARGET_LANGUAGE}")
		else()
			cotire_generate_unity_source(${_files})
		endif()

	elseif ("${COTIRE_ARGV1}" STREQUAL "cleanup")

		cotire_cleanup("${COTIRE_ARGV2}" "${COTIRE_ARGV3}" "${COTIRE_ARGV4}")

	else()
		message (FATAL_ERROR "Unknown cotire command \"${COTIRE_ARGV1}\".")
	endif()

else()

	# cotire is being run in include mode
	# set up all variable and property definitions

	unset (COTIRE_C_COMPILER_VERSION CACHE)
	unset (COTIRE_CXX_COMPILER_VERSION CACHE)

	if (NOT DEFINED COTIRE_DEBUG_INIT)
		if (DEFINED COTIRE_DEBUG)
			set (COTIRE_DEBUG_INIT ${COTIRE_DEBUG})
		else()
			set (COTIRE_DEBUG_INIT FALSE)
		endif()
	endif()
	option (COTIRE_DEBUG "Enable cotire debugging output?" ${COTIRE_DEBUG_INIT})

	if (NOT DEFINED COTIRE_VERBOSE_INIT)
		if (DEFINED COTIRE_VERBOSE)
			set (COTIRE_VERBOSE_INIT ${COTIRE_VERBOSE})
		else()
			set (COTIRE_VERBOSE_INIT FALSE)
		endif()
	endif()
	option (COTIRE_VERBOSE "Enable cotire verbose output?" ${COTIRE_VERBOSE_INIT})

	set (COTIRE_ADDITIONAL_PREFIX_HEADER_IGNORE_EXTENSIONS "inc;inl;ipp" CACHE STRING
		"Ignore headers with the listed file extensions from the generated prefix header.")

	set (COTIRE_ADDITIONAL_PREFIX_HEADER_IGNORE_PATH "" CACHE STRING
		"Ignore headers from these directories when generating the prefix header.")

	set (COTIRE_UNITY_SOURCE_EXCLUDE_EXTENSIONS "m;mm" CACHE STRING
		"Ignore sources with the listed file extensions from the generated unity source.")

	set (COTIRE_MINIMUM_NUMBER_OF_TARGET_SOURCES "3" CACHE STRING
		"Minimum number of sources in target required to enable use of precompiled header.")

	if (NOT DEFINED COTIRE_MAXIMUM_NUMBER_OF_UNITY_INCLUDES_INIT)
		if (DEFINED COTIRE_MAXIMUM_NUMBER_OF_UNITY_INCLUDES)
			set (COTIRE_MAXIMUM_NUMBER_OF_UNITY_INCLUDES_INIT ${COTIRE_MAXIMUM_NUMBER_OF_UNITY_INCLUDES})
		elseif ("${CMAKE_GENERATOR}" MATCHES "JOM|Ninja|Visual Studio")
			# enable parallelization for generators that run multiple jobs by default
			set (COTIRE_MAXIMUM_NUMBER_OF_UNITY_INCLUDES_INIT "-j")
		else()
			set (COTIRE_MAXIMUM_NUMBER_OF_UNITY_INCLUDES_INIT "0")
		endif()
	endif()
	set (COTIRE_MAXIMUM_NUMBER_OF_UNITY_INCLUDES "${COTIRE_MAXIMUM_NUMBER_OF_UNITY_INCLUDES_INIT}" CACHE STRING
		"Maximum number of source files to include in a single unity source file.")

	if (NOT COTIRE_PREFIX_HEADER_FILENAME_SUFFIX)
		set (COTIRE_PREFIX_HEADER_FILENAME_SUFFIX "_prefix")
	endif()
	if (NOT COTIRE_UNITY_SOURCE_FILENAME_SUFFIX)
		set (COTIRE_UNITY_SOURCE_FILENAME_SUFFIX "_unity")
	endif()
	if (NOT COTIRE_INTDIR)
		set (COTIRE_INTDIR "cotire")
	endif()
	if (NOT COTIRE_PCH_ALL_TARGET_NAME)
		set (COTIRE_PCH_ALL_TARGET_NAME "all_pch")
	endif()
	if (NOT COTIRE_UNITY_BUILD_ALL_TARGET_NAME)
		set (COTIRE_UNITY_BUILD_ALL_TARGET_NAME "all_unity")
	endif()
	if (NOT COTIRE_CLEAN_ALL_TARGET_NAME)
		set (COTIRE_CLEAN_ALL_TARGET_NAME "clean_cotire")
	endif()
	if (NOT COTIRE_CLEAN_TARGET_SUFFIX)
		set (COTIRE_CLEAN_TARGET_SUFFIX "_clean_cotire")
	endif()
	if (NOT COTIRE_PCH_TARGET_SUFFIX)
		set (COTIRE_PCH_TARGET_SUFFIX "_pch")
	endif()
	if (NOT COTIRE_UNITY_BUILD_TARGET_SUFFIX)
		set (COTIRE_UNITY_BUILD_TARGET_SUFFIX "_unity")
	endif()
	if (NOT DEFINED COTIRE_TARGETS_FOLDER)
		set (COTIRE_TARGETS_FOLDER "cotire")
	endif()
	if (NOT DEFINED COTIRE_UNITY_OUTPUT_DIRECTORY)
		if ("${CMAKE_GENERATOR}" MATCHES "Ninja")
			# generated Ninja build files do not work if the unity target produces the same output file as the cotired target
			set (COTIRE_UNITY_OUTPUT_DIRECTORY "unity")
		else()
			set (COTIRE_UNITY_OUTPUT_DIRECTORY "")
		endif()
	endif()

	# define cotire cache variables

	define_property(
		CACHED_VARIABLE PROPERTY "COTIRE_ADDITIONAL_PREFIX_HEADER_IGNORE_PATH"
		BRIEF_DOCS "Ignore headers from these directories when generating the prefix header."
		FULL_DOCS
			"The variable can be set to a semicolon separated list of include directories."
			"If a header file is found in one of these directories or sub-directories, it will be excluded from the generated prefix header."
			"If not defined, defaults to empty list."
	)

	define_property(
		CACHED_VARIABLE PROPERTY "COTIRE_ADDITIONAL_PREFIX_HEADER_IGNORE_EXTENSIONS"
		BRIEF_DOCS "Ignore includes with the listed file extensions from the generated prefix header."
		FULL_DOCS
			"The variable can be set to a semicolon separated list of file extensions."
			"If a header file extension matches one in the list, it will be excluded from the generated prefix header."
			"Includes with an extension in CMAKE_<LANG>_SOURCE_FILE_EXTENSIONS are always ignored."
			"If not defined, defaults to inc;inl;ipp."
	)

	define_property(
		CACHED_VARIABLE PROPERTY "COTIRE_UNITY_SOURCE_EXCLUDE_EXTENSIONS"
		BRIEF_DOCS "Exclude sources with the listed file extensions from the generated unity source."
		FULL_DOCS
			"The variable can be set to a semicolon separated list of file extensions."
			"If a source file extension matches one in the list, it will be excluded from the generated unity source file."
			"Source files with an extension in CMAKE_<LANG>_IGNORE_EXTENSIONS are always excluded."
			"If not defined, defaults to m;mm."
	)

	define_property(
		CACHED_VARIABLE PROPERTY "COTIRE_MINIMUM_NUMBER_OF_TARGET_SOURCES"
		BRIEF_DOCS "Minimum number of sources in target required to enable use of precompiled header."
		FULL_DOCS
			"The variable can be set to an integer > 0."
			"If a target contains less than that number of source files, cotire will not enable the use of the precompiled header for the target."
			"If not defined, defaults to 3."
	)

	define_property(
		CACHED_VARIABLE PROPERTY "COTIRE_MAXIMUM_NUMBER_OF_UNITY_INCLUDES"
		BRIEF_DOCS "Maximum number of source files to include in a single unity source file."
		FULL_DOCS
			"This may be set to an integer >= 0."
			"If 0, cotire will only create a single unity source file."
			"If a target contains more than that number of source files, cotire will create multiple unity source files for it."
			"Can be set to \"-j\" to optimize the count of unity source files for the number of available processor cores."
			"Can be set to \"-j jobs\" to optimize the number of unity source files for the given number of simultaneous jobs."
			"Is used to initialize the target property COTIRE_UNITY_SOURCE_MAXIMUM_NUMBER_OF_INCLUDES."
			"Defaults to \"-j\" for the generators Visual Studio, JOM or Ninja. Defaults to 0 otherwise."
	)

	# define cotire directory properties

	define_property(
		DIRECTORY PROPERTY "COTIRE_ENABLE_PRECOMPILED_HEADER"
		BRIEF_DOCS "Modify build command of cotired targets added in this directory to make use of the generated precompiled header."
		FULL_DOCS
			"See target property COTIRE_ENABLE_PRECOMPILED_HEADER."
	)

	define_property(
		DIRECTORY PROPERTY "COTIRE_ADD_UNITY_BUILD"
		BRIEF_DOCS "Add a new target that performs a unity build for cotired targets added in this directory."
		FULL_DOCS
			"See target property COTIRE_ADD_UNITY_BUILD."
	)

	define_property(
		DIRECTORY PROPERTY "COTIRE_ADD_CLEAN"
		BRIEF_DOCS "Add a new target that cleans all cotire generated files for cotired targets added in this directory."
		FULL_DOCS
			"See target property COTIRE_ADD_CLEAN."
	)

	define_property(
		DIRECTORY PROPERTY "COTIRE_PREFIX_HEADER_IGNORE_PATH"
		BRIEF_DOCS "Ignore headers from these directories when generating the prefix header."
		FULL_DOCS
			"See target property COTIRE_PREFIX_HEADER_IGNORE_PATH."
	)

	define_property(
		DIRECTORY PROPERTY "COTIRE_PREFIX_HEADER_INCLUDE_PATH"
		BRIEF_DOCS "Honor headers from these directories when generating the prefix header."
		FULL_DOCS
			"See target property COTIRE_PREFIX_HEADER_INCLUDE_PATH."
	)

	define_property(
		DIRECTORY PROPERTY "COTIRE_UNITY_SOURCE_PRE_UNDEFS"
		BRIEF_DOCS "Preprocessor undefs to place in the generated unity source file before the inclusion of each source file."
		FULL_DOCS
			"See target property COTIRE_UNITY_SOURCE_PRE_UNDEFS."
	)

	define_property(
		DIRECTORY PROPERTY "COTIRE_UNITY_SOURCE_POST_UNDEFS"
		BRIEF_DOCS "Preprocessor undefs to place in the generated unity source file after the inclusion of each source file."
		FULL_DOCS
			"See target property COTIRE_UNITY_SOURCE_POST_UNDEFS."
	)

	define_property(
		DIRECTORY PROPERTY "COTIRE_UNITY_SOURCE_MAXIMUM_NUMBER_OF_INCLUDES"
		BRIEF_DOCS "Maximum number of source files to include in a single unity source file."
		FULL_DOCS
			"See target property COTIRE_UNITY_SOURCE_MAXIMUM_NUMBER_OF_INCLUDES."
	)

	define_property(
		DIRECTORY PROPERTY "COTIRE_UNITY_LINK_LIBRARIES_INIT"
		BRIEF_DOCS "Define strategy for setting up the unity target's link libraries."
		FULL_DOCS
			"See target property COTIRE_UNITY_LINK_LIBRARIES_INIT."
	)

	# define cotire target properties

	define_property(
		TARGET PROPERTY "COTIRE_ENABLE_PRECOMPILED_HEADER" INHERITED
		BRIEF_DOCS "Modify this target's build command to make use of the generated precompiled header."
		FULL_DOCS
			"If this property is set to TRUE, cotire will modify the build command to make use of the generated precompiled header."
			"Irrespective of the value of this property, cotire will setup custom commands to generate the unity source and prefix header for the target."
			"For makefile based generators cotire will also set up a custom target to manually invoke the generation of the precompiled header."
			"The target name will be set to this target's name with the suffix _pch appended."
			"Inherited from directory."
			"Defaults to TRUE."
	)

	define_property(
		TARGET PROPERTY "COTIRE_ADD_UNITY_BUILD" INHERITED
		BRIEF_DOCS "Add a new target that performs a unity build for this target."
		FULL_DOCS
			"If this property is set to TRUE, cotire creates a new target of the same type that uses the generated unity source file instead of the target sources."
			"Most of the relevant target properties will be copied from this target to the new unity build target."
			"Target dependencies and linked libraries have to be manually set up for the new unity build target."
			"The unity target name will be set to this target's name with the suffix _unity appended."
			"Inherited from directory."
			"Defaults to TRUE."
	)

	define_property(
		TARGET PROPERTY "COTIRE_ADD_CLEAN" INHERITED
		BRIEF_DOCS "Add a new target that cleans all cotire generated files for this target."
		FULL_DOCS
			"If this property is set to TRUE, cotire creates a new target that clean all files (unity source, prefix header, precompiled header)."
			"The clean target name will be set to this target's name with the suffix _clean_cotire appended."
			"Inherited from directory."
			"Defaults to FALSE."
	)

	define_property(
		TARGET PROPERTY "COTIRE_PREFIX_HEADER_IGNORE_PATH" INHERITED
		BRIEF_DOCS "Ignore headers from these directories when generating the prefix header."
		FULL_DOCS
			"The property can be set to a list of directories."
			"If a header file is found in one of these directories or sub-directories, it will be excluded from the generated prefix header."
			"Inherited from directory."
			"If not set, this property is initialized to \${CMAKE_SOURCE_DIR};\${CMAKE_BINARY_DIR}."
	)

	define_property(
		TARGET PROPERTY "COTIRE_PREFIX_HEADER_INCLUDE_PATH" INHERITED
		BRIEF_DOCS "Honor headers from these directories when generating the prefix header."
		FULL_DOCS
			"The property can be set to a list of directories."
			"If a header file is found in one of these directories or sub-directories, it will be included in the generated prefix header."
			"If a header file is both selected by COTIRE_PREFIX_HEADER_IGNORE_PATH and COTIRE_PREFIX_HEADER_INCLUDE_PATH,"
			"the option which yields the closer relative path match wins."
			"Inherited from directory."
			"If not set, this property is initialized to the empty list."
	)

	define_property(
		TARGET PROPERTY "COTIRE_UNITY_SOURCE_PRE_UNDEFS" INHERITED
		BRIEF_DOCS "Preprocessor undefs to place in the generated unity source file before the inclusion of each target source file."
		FULL_DOCS
			"This may be set to a semicolon-separated list of preprocessor symbols."
			"cotire will add corresponding #undef directives to the generated unit source file before each target source file."
			"Inherited from directory."
			"Defaults to empty string."
	)

	define_property(
		TARGET PROPERTY "COTIRE_UNITY_SOURCE_POST_UNDEFS" INHERITED
		BRIEF_DOCS "Preprocessor undefs to place in the generated unity source file after the inclusion of each target source file."
		FULL_DOCS
			"This may be set to a semicolon-separated list of preprocessor symbols."
			"cotire will add corresponding #undef directives to the generated unit source file after each target source file."
			"Inherited from directory."
			"Defaults to empty string."
	)

	define_property(
		TARGET PROPERTY "COTIRE_UNITY_SOURCE_MAXIMUM_NUMBER_OF_INCLUDES" INHERITED
		BRIEF_DOCS "Maximum number of source files to include in a single unity source file."
		FULL_DOCS
			"This may be set to an integer > 0."
			"If a target contains more than that number of source files, cotire will create multiple unity build files for it."
			"If not set, cotire will only create a single unity source file."
			"Inherited from directory."
			"Defaults to empty."
	)

	define_property(
		TARGET PROPERTY "COTIRE_<LANG>_UNITY_SOURCE_INIT"
		BRIEF_DOCS "User provided unity source file to be used instead of the automatically generated one."
		FULL_DOCS
			"If set, cotire will only add the given file(s) to the generated unity source file."
			"If not set, cotire will add all the target source files to the generated unity source file."
			"The property can be set to a user provided unity source file."
			"Defaults to empty."
	)

	define_property(
		TARGET PROPERTY "COTIRE_<LANG>_PREFIX_HEADER_INIT"
		BRIEF_DOCS "User provided prefix header file to be used instead of the automatically generated one."
		FULL_DOCS
			"If set, cotire will add the given header file(s) to the generated prefix header file."
			"If not set, cotire will generate a prefix header by tracking the header files included by the unity source file."
			"The property can be set to a user provided prefix header file (e.g., stdafx.h)."
			"Defaults to empty."
	)

	define_property(
		TARGET PROPERTY "COTIRE_UNITY_LINK_LIBRARIES_INIT" INHERITED
		BRIEF_DOCS "Define strategy for setting up unity target's link libraries."
		FULL_DOCS
			"If this property is empty, the generated unity target's link libraries have to be set up manually."
			"If this property is set to COPY, the unity target's link libraries will be copied from this target."
			"If this property is set to COPY_UNITY, the unity target's link libraries will be copied from this target with considering existing unity targets."
			"Inherited from directory."
			"Defaults to empty."
	)

	define_property(
		TARGET PROPERTY "COTIRE_<LANG>_UNITY_SOURCE"
		BRIEF_DOCS "Read-only property. The generated <LANG> unity source file(s)."
		FULL_DOCS
			"cotire sets this property to the path of the generated <LANG> single computation unit source file for the target."
			"Defaults to empty string."
	)

	define_property(
		TARGET PROPERTY "COTIRE_<LANG>_PREFIX_HEADER"
		BRIEF_DOCS "Read-only property. The generated <LANG> prefix header file."
		FULL_DOCS
			"cotire sets this property to the full path of the generated <LANG> language prefix header for the target."
			"Defaults to empty string."
	)

	define_property(
		TARGET PROPERTY "COTIRE_<LANG>_PRECOMPILED_HEADER"
		BRIEF_DOCS "Read-only property. The generated <LANG> precompiled header file."
		FULL_DOCS
			"cotire sets this property to the full path of the generated <LANG> language precompiled header binary for the target."
			"Defaults to empty string."
	)

	define_property(
		TARGET PROPERTY "COTIRE_UNITY_TARGET_NAME"
		BRIEF_DOCS "The name of the generated unity build target corresponding to this target."
		FULL_DOCS
			"This property can be set to the desired name of the unity target that will be created by cotire."
			"If not set, the unity target name will be set to this target's name with the suffix _unity appended."
			"After this target has been processed by cotire, the property is set to the actual name of the generated unity target."
			"Defaults to empty string."
	)

	# define cotire source properties

	define_property(
		SOURCE PROPERTY "COTIRE_EXCLUDED"
		BRIEF_DOCS "Do not modify source file's build command."
		FULL_DOCS
			"If this property is set to TRUE, the source file's build command will not be modified to make use of the precompiled header."
			"The source file will also be excluded from the generated unity source file."
			"Source files that have their COMPILE_FLAGS property set will be excluded by default."
			"Defaults to FALSE."
	)

	define_property(
		SOURCE PROPERTY "COTIRE_DEPENDENCY"
		BRIEF_DOCS "Add this source file to dependencies of the automatically generated prefix header file."
		FULL_DOCS
			"If this property is set to TRUE, the source file is added to dependencies of the generated prefix header file."
			"If the file is modified, cotire will re-generate the prefix header source upon build."
			"Defaults to FALSE."
	)

	define_property(
		SOURCE PROPERTY "COTIRE_UNITY_SOURCE_PRE_UNDEFS"
		BRIEF_DOCS "Preprocessor undefs to place in the generated unity source file before the inclusion of this source file."
		FULL_DOCS
			"This may be set to a semicolon-separated list of preprocessor symbols."
			"cotire will add corresponding #undef directives to the generated unit source file before this file is included."
			"Defaults to empty string."
	)

	define_property(
		SOURCE PROPERTY "COTIRE_UNITY_SOURCE_POST_UNDEFS"
		BRIEF_DOCS "Preprocessor undefs to place in the generated unity source file after the inclusion of this source file."
		FULL_DOCS
			"This may be set to a semicolon-separated list of preprocessor symbols."
			"cotire will add corresponding #undef directives to the generated unit source file after this file is included."
			"Defaults to empty string."
	)

	define_property(
		SOURCE PROPERTY "COTIRE_START_NEW_UNITY_SOURCE"
		BRIEF_DOCS "Start a new unity source file which includes this source file as the first one."
		FULL_DOCS
			"If this property is set to TRUE, cotire will complete the current unity file and start a new one."
			"The new unity source file will include this source file as the first one."
			"This property essentially works as a separator for unity source files."
			"Defaults to FALSE."
	)

	define_property(
		SOURCE PROPERTY "COTIRE_TARGET"
		BRIEF_DOCS "Read-only property. Mark this source file as cotired for the given target."
		FULL_DOCS
			"cotire sets this property to the name of target, that the source file's build command has been altered for."
			"Defaults to empty string."
	)

	message (STATUS "cotire ${COTIRE_CMAKE_MODULE_VERSION} loaded.")

endif()
