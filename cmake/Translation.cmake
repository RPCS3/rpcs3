# Macro to compile po file
# It based on FindGettext.cmake files.
# The macro was adapted for PCSX2 need. Several pot file, language based on directory instead of file

# Copyright (c) 2007-2009 Kitware, Inc., Insight Consortium
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#  * Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
#
#  * Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
#  * The names of Kitware, Inc., the Insight Consortium, or the names of
#    any consortium members, or of any contributors, may not be used to
#    endorse or promote products derived from this software without
#    specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND CONTRIBUTORS ``AS IS''
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

MACRO(GETTEXT_CREATE_TRANSLATIONS_PCSX2 _potFile _firstPoFileArg)
    # make it a real variable, so we can modify it here
    SET(_firstPoFile "${_firstPoFileArg}")

    SET(_gmoFiles)
    GET_FILENAME_COMPONENT(_potBasename ${_potFile} NAME_WE)
    GET_FILENAME_COMPONENT(_absPotFile ${_potFile} ABSOLUTE)

    SET(_addToAll)
    IF(${_firstPoFile} STREQUAL "ALL")
        SET(_addToAll "ALL")
        SET(_firstPoFile)
    ENDIF(${_firstPoFile} STREQUAL "ALL")

    FOREACH (_currentPoFile ${_firstPoFile} ${ARGN})
        GET_FILENAME_COMPONENT(_absFile ${_currentPoFile} ABSOLUTE)
        GET_FILENAME_COMPONENT(_abs_PATH ${_absFile} PATH)
        GET_FILENAME_COMPONENT(_gmoBase ${_absFile} NAME_WE)
        GET_FILENAME_COMPONENT(_lang ${_abs_PATH} NAME_WE)
        SET(_gmoFile ${CMAKE_CURRENT_BINARY_DIR}/${_lang}__${_gmoBase}.gmo)

        ADD_CUSTOM_COMMAND( OUTPUT ${_gmoFile}
            COMMAND ${GETTEXT_MSGMERGE_EXECUTABLE} --quiet --update --backup=none -s ${_absFile} ${_absPotFile}
            COMMAND ${GETTEXT_MSGFMT_EXECUTABLE} -o ${_gmoFile} ${_absFile}
            DEPENDS ${_absPotFile} ${_absFile} )

        INSTALL(FILES ${_gmoFile} DESTINATION share/locale/${_lang}/LC_MESSAGES RENAME ${_potBasename}.mo)

        SET(_gmoFiles ${_gmoFiles} ${_gmoFile})

    ENDFOREACH (_currentPoFile )

    IF(NOT LINUX_PACKAGE)
        ADD_CUSTOM_TARGET(translations_${_potBasename} ${_addToAll} DEPENDS ${_gmoFiles})
    ENDIF(NOT LINUX_PACKAGE)

ENDMACRO(GETTEXT_CREATE_TRANSLATIONS_PCSX2 )
