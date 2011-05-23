# FIND_PACKAGE_HANDLE_STANDARD_ARGS(<name> ... )
#
# This function is intended to be used in FindXXX.cmake modules files.
# It handles the REQUIRED, QUIET and version-related arguments to FIND_PACKAGE().
# It also sets the <UPPERCASED_NAME>_FOUND variable.
# The package is considered found if all variables <var1>... listed contain
# valid results, e.g. valid filepaths.
#
# There are two modes of this function. The first argument in both modes is
# the name of the Find-module where it is called (in original casing).
#
# The first simple mode looks like this:
#    FIND_PACKAGE_HANDLE_STANDARD_ARGS(<name> (DEFAULT_MSG|"Custom failure message") <var1>...<varN> )
# If the variables <var1> to <varN> are all valid, then <UPPERCASED_NAME>_FOUND
# will be set to TRUE.
# If DEFAULT_MSG is given as second argument, then the function will generate
# itself useful success and error messages. You can also supply a custom error message
# for the failure case. This is not recommended.
#
# The second mode is more powerful and also supports version checking:
#    FIND_PACKAGE_HANDLE_STANDARD_ARGS(NAME [REQUIRED_VARS <var1>...<varN>]
#                                           [VERSION_VAR   <versionvar>
#                                           [CONFIG_MODE]
#                                           [FAIL_MESSAGE "Custom failure message"] )
#
# As above, if <var1> through <varN> are all valid, <UPPERCASED_NAME>_FOUND
# will be set to TRUE.
# After REQUIRED_VARS the variables which are required for this package are listed.
# Following VERSION_VAR the name of the variable can be specified which holds
# the version of the package which has been found. If this is done, this version
# will be checked against the (potentially) specified required version used
# in the find_package() call. The EXACT keyword is also handled. The default
# messages include information about the required version and the version
# which has been actually found, both if the version is ok or not.
# Use the option CONFIG_MODE if your FindXXX.cmake module is a wrapper for
# a find_package(... NO_MODULE) call, in this case all the information
# provided by the config-mode of find_package() will be evaluated
# automatically.
# Via FAIL_MESSAGE a custom failure message can be specified, if this is not
# used, the default message will be displayed.
#
# Example for mode 1:
#
#    FIND_PACKAGE_HANDLE_STANDARD_ARGS(LibXml2  DEFAULT_MSG  LIBXML2_LIBRARY LIBXML2_INCLUDE_DIR)
#
# LibXml2 is considered to be found, if both LIBXML2_LIBRARY and
# LIBXML2_INCLUDE_DIR are valid. Then also LIBXML2_FOUND is set to TRUE.
# If it is not found and REQUIRED was used, it fails with FATAL_ERROR,
# independent whether QUIET was used or not.
# If it is found, success will be reported, including the content of <var1>.
# On repeated Cmake runs, the same message won't be printed again.
#
# Example for mode 2:
#
#    FIND_PACKAGE_HANDLE_STANDARD_ARGS(BISON  REQUIRED_VARS BISON_EXECUTABLE
#                                             VERSION_VAR BISON_VERSION)
# In this case, BISON is considered to be found if the variable(s) listed
# after REQUIRED_VAR are all valid, i.e. BISON_EXECUTABLE in this case.
# Also the version of BISON will be checked by using the version contained
# in BISON_VERSION.
# Since no FAIL_MESSAGE is given, the default messages will be printed.
#
# Another example for mode 2:
#
#    FIND_PACKAGE(Automoc4 QUIET NO_MODULE HINTS /opt/automoc4)
#    FIND_PACKAGE_HANDLE_STANDARD_ARGS(Automoc4  CONFIG_MODE)
# In this case, FindAutmoc4.cmake wraps a call to FIND_PACKAGE(Automoc4 NO_MODULE)
# and adds an additional search directory for automoc4.
# The following FIND_PACKAGE_HANDLE_STANDARD_ARGS() call produces a proper
# success/error message.

#=============================================================================
# Copyright 2007-2009 Kitware, Inc.
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distribute this file outside of CMake, substitute the full
#  License text for the above reference.)

INCLUDE(FindPackageMessage)
INCLUDE(CMakeParseArguments)

# internal helper macro
MACRO(_FPHSA_FAILURE_MESSAGE _msg)
  IF (${_NAME}_FIND_REQUIRED)
    MESSAGE(FATAL_ERROR "${_msg}")
  ELSE (${_NAME}_FIND_REQUIRED)
    IF (NOT ${_NAME}_FIND_QUIETLY)
      MESSAGE(STATUS "${_msg}")
    ENDIF (NOT ${_NAME}_FIND_QUIETLY)
  ENDIF (${_NAME}_FIND_REQUIRED)
ENDMACRO(_FPHSA_FAILURE_MESSAGE _msg)


# internal helper macro to generate the failure message when used in CONFIG_MODE:
MACRO(_FPHSA_HANDLE_FAILURE_CONFIG_MODE)
  # <name>_CONFIG is set, but FOUND is false, this means that some other of the REQUIRED_VARS was not found:
  IF(${_NAME}_CONFIG)
    _FPHSA_FAILURE_MESSAGE("${FPHSA_FAIL_MESSAGE}: missing: ${MISSING_VARS} (found ${${_NAME}_CONFIG} ${VERSION_MSG})")
  ELSE(${_NAME}_CONFIG)
    # If _CONSIDERED_CONFIGS is set, the config-file has been found, but no suitable version.
    # List them all in the error message:
    IF(${_NAME}_CONSIDERED_CONFIGS)
      SET(configsText "")
      LIST(LENGTH ${_NAME}_CONSIDERED_CONFIGS configsCount)
      MATH(EXPR configsCount "${configsCount} - 1")
      FOREACH(currentConfigIndex RANGE ${configsCount})
        LIST(GET ${_NAME}_CONSIDERED_CONFIGS ${currentConfigIndex} filename)
        LIST(GET ${_NAME}_CONSIDERED_VERSIONS ${currentConfigIndex} version)
        SET(configsText "${configsText}    ${filename} (version ${version})\n")
      ENDFOREACH(currentConfigIndex)
      _FPHSA_FAILURE_MESSAGE("${FPHSA_FAIL_MESSAGE} ${VERSION_MSG}, checked the following files:\n${configsText}")

    ELSE(${_NAME}_CONSIDERED_CONFIGS)
      # Simple case: No Config-file was found at all:
      _FPHSA_FAILURE_MESSAGE("${FPHSA_FAIL_MESSAGE}: found neither ${_NAME}Config.cmake nor ${_NAME_LOWER}-config.cmake ${VERSION_MSG}")
    ENDIF(${_NAME}_CONSIDERED_CONFIGS)
  ENDIF(${_NAME}_CONFIG)
ENDMACRO(_FPHSA_HANDLE_FAILURE_CONFIG_MODE)


FUNCTION(FIND_PACKAGE_HANDLE_STANDARD_ARGS _NAME _FIRST_ARG)

# set up the arguments for CMAKE_PARSE_ARGUMENTS and check whether we are in
# new extended or in the "old" mode:
  SET(options CONFIG_MODE)
  SET(oneValueArgs FAIL_MESSAGE VERSION_VAR)
  SET(multiValueArgs REQUIRED_VARS)
  SET(_KEYWORDS_FOR_EXTENDED_MODE  ${options} ${oneValueArgs} ${multiValueArgs} )
  LIST(FIND _KEYWORDS_FOR_EXTENDED_MODE "${_FIRST_ARG}" INDEX)

  IF(${INDEX} EQUAL -1)
    SET(FPHSA_FAIL_MESSAGE ${_FIRST_ARG})
    SET(FPHSA_REQUIRED_VARS ${ARGN})
    SET(FPHSA_VERSION_VAR)
  ELSE(${INDEX} EQUAL -1)

    CMAKE_PARSE_ARGUMENTS(FPHSA "${options}" "${oneValueArgs}" "${multiValueArgs}"  ${_FIRST_ARG} ${ARGN})

    IF(FPHSA_UNPARSED_ARGUMENTS)
      MESSAGE(FATAL_ERROR "Unknown keywords given to FIND_PACKAGE_HANDLE_STANDARD_ARGS(): \"${FPHSA_UNPARSED_ARGUMENTS}\"")
    ENDIF(FPHSA_UNPARSED_ARGUMENTS)

    IF(NOT FPHSA_FAIL_MESSAGE)
      SET(FPHSA_FAIL_MESSAGE  "DEFAULT_MSG")
    ENDIF(NOT FPHSA_FAIL_MESSAGE)
  ENDIF(${INDEX} EQUAL -1)

# now that we collected all arguments, process them

  IF("${FPHSA_FAIL_MESSAGE}" STREQUAL "DEFAULT_MSG")
    SET(FPHSA_FAIL_MESSAGE "Could NOT find ${_NAME}")
  ENDIF("${FPHSA_FAIL_MESSAGE}" STREQUAL "DEFAULT_MSG")

  # In config-mode, we rely on the variable <package>_CONFIG, which is set by find_package()
  # when it successfully found the config-file, including version checking:
  IF(FPHSA_CONFIG_MODE)
    LIST(INSERT FPHSA_REQUIRED_VARS 0 ${_NAME}_CONFIG)
    LIST(REMOVE_DUPLICATES FPHSA_REQUIRED_VARS)
    SET(FPHSA_VERSION_VAR ${_NAME}_VERSION)
  ENDIF(FPHSA_CONFIG_MODE)

  IF(NOT FPHSA_REQUIRED_VARS)
    MESSAGE(FATAL_ERROR "No REQUIRED_VARS specified for FIND_PACKAGE_HANDLE_STANDARD_ARGS()")
  ENDIF(NOT FPHSA_REQUIRED_VARS)

  LIST(GET FPHSA_REQUIRED_VARS 0 _FIRST_REQUIRED_VAR)

  STRING(TOUPPER ${_NAME} _NAME_UPPER)
  STRING(TOLOWER ${_NAME} _NAME_LOWER)

  # collect all variables which were not found, so they can be printed, so the
  # user knows better what went wrong (#6375)
  SET(MISSING_VARS "")
  SET(DETAILS "")
  SET(${_NAME_UPPER}_FOUND TRUE)
  # check if all passed variables are valid
  FOREACH(_CURRENT_VAR ${FPHSA_REQUIRED_VARS})
    IF(NOT ${_CURRENT_VAR})
      SET(${_NAME_UPPER}_FOUND FALSE)
      SET(MISSING_VARS "${MISSING_VARS} ${_CURRENT_VAR}")
    ELSE(NOT ${_CURRENT_VAR})
      SET(DETAILS "${DETAILS}[${${_CURRENT_VAR}}]")
    ENDIF(NOT ${_CURRENT_VAR})
  ENDFOREACH(_CURRENT_VAR)


  # version handling:
  SET(VERSION_MSG "")
  SET(VERSION_OK TRUE)
  SET(VERSION ${${FPHSA_VERSION_VAR}} )
  IF (${_NAME}_FIND_VERSION)

    IF(VERSION)

      IF(${_NAME}_FIND_VERSION_EXACT)       # exact version required
        IF (NOT "${${_NAME}_FIND_VERSION}" VERSION_EQUAL "${VERSION}")
          SET(VERSION_MSG "Found unsuitable version \"${VERSION}\", but required is exact version \"${${_NAME}_FIND_VERSION}\"")
          SET(VERSION_OK FALSE)
        ELSE (NOT "${${_NAME}_FIND_VERSION}" VERSION_EQUAL "${VERSION}")
          SET(VERSION_MSG "(found suitable exact version \"${VERSION}\")")
        ENDIF (NOT "${${_NAME}_FIND_VERSION}" VERSION_EQUAL "${VERSION}")

      ELSE(${_NAME}_FIND_VERSION_EXACT)     # minimum version specified:
        IF ("${${_NAME}_FIND_VERSION}" VERSION_GREATER "${VERSION}")
          SET(VERSION_MSG "Found unsuitable version \"${VERSION}\", but required is at least \"${${_NAME}_FIND_VERSION}\"")
          SET(VERSION_OK FALSE)
        ELSE ("${${_NAME}_FIND_VERSION}" VERSION_GREATER "${VERSION}")
          SET(VERSION_MSG "(found suitable version \"${VERSION}\", required is \"${${_NAME}_FIND_VERSION}\")")
        ENDIF ("${${_NAME}_FIND_VERSION}" VERSION_GREATER "${VERSION}")
      ENDIF(${_NAME}_FIND_VERSION_EXACT)

    ELSE(VERSION)

      # if the package was not found, but a version was given, add that to the output:
      IF(${_NAME}_FIND_VERSION_EXACT)
         SET(VERSION_MSG "(Required is exact version \"${${_NAME}_FIND_VERSION}\")")
      ELSE(${_NAME}_FIND_VERSION_EXACT)
         SET(VERSION_MSG "(Required is at least version \"${${_NAME}_FIND_VERSION}\")")
      ENDIF(${_NAME}_FIND_VERSION_EXACT)

    ENDIF(VERSION)
  ELSE (${_NAME}_FIND_VERSION)
    IF(VERSION)
      SET(VERSION_MSG "(found version \"${VERSION}\")")
    ENDIF(VERSION)
  ENDIF (${_NAME}_FIND_VERSION)

  IF(VERSION_OK)
    SET(DETAILS "${DETAILS}[v${VERSION}(${${_NAME}_FIND_VERSION})]")
  ELSE(VERSION_OK)
    SET(${_NAME_UPPER}_FOUND FALSE)
  ENDIF(VERSION_OK)


  # print the result:
  IF (${_NAME_UPPER}_FOUND)
    FIND_PACKAGE_MESSAGE(${_NAME} "Found ${_NAME}: ${${_FIRST_REQUIRED_VAR}} ${VERSION_MSG}" "${DETAILS}")
  ELSE (${_NAME_UPPER}_FOUND)

    IF(FPHSA_CONFIG_MODE)
      _FPHSA_HANDLE_FAILURE_CONFIG_MODE()
    ELSE(FPHSA_CONFIG_MODE)
      IF(NOT VERSION_OK)
        _FPHSA_FAILURE_MESSAGE("${FPHSA_FAIL_MESSAGE}: ${VERSION_MSG} (found ${${_FIRST_REQUIRED_VAR}})")
      ELSE(NOT VERSION_OK)
        _FPHSA_FAILURE_MESSAGE("${FPHSA_FAIL_MESSAGE} (missing: ${MISSING_VARS}) ${VERSION_MSG}")
      ENDIF(NOT VERSION_OK)
    ENDIF(FPHSA_CONFIG_MODE)

  ENDIF (${_NAME_UPPER}_FOUND)

  SET(${_NAME_UPPER}_FOUND ${${_NAME_UPPER}_FOUND} PARENT_SCOPE)

ENDFUNCTION(FIND_PACKAGE_HANDLE_STANDARD_ARGS _FIRST_ARG)
