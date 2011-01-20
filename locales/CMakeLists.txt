# Check that people use the good file
if(NOT TOP_CMAKE_WAS_SOURCED)
    message(FATAL_ERROR "
    You did not 'cmake' the good CMakeLists.txt file. Use the one in the top dir.
    It is advice to delete all wrongly generated cmake stuff => CMakeFiles & CMakeCache.txt")
endif(NOT TOP_CMAKE_WAS_SOURCED)

if (GETTEXT_FOUND)
    include(Translation)

    file (GLOB_RECURSE PO_DEV_FILES ${PROJECT_SOURCE_DIR}}/locales  pcsx2_Devel.po)
    file (GLOB_RECURSE PO_ICO_FILES ${PROJECT_SOURCE_DIR}}/locales  pcsx2_Iconized.po)
    file (GLOB_RECURSE PO_MAIN_FILES ${PROJECT_SOURCE_DIR}/locales  pcsx2_Main.po)
    file (GLOB_RECURSE PO_TER_FILES ${PROJECT_SOURCE_DIR}}/locales  pcsx2_Tertiary.po)

    # Macro to compile po file and install them
    GETTEXT_CREATE_TRANSLATIONS_PCSX2(${PROJECT_SOURCE_DIR}/locales/templates/pcsx2_Devel.pot    ALL ${PO_DEV_FILES})
    GETTEXT_CREATE_TRANSLATIONS_PCSX2(${PROJECT_SOURCE_DIR}/locales/templates/pcsx2_Iconized.pot ALL ${PO_ICO_FILES})
    GETTEXT_CREATE_TRANSLATIONS_PCSX2(${PROJECT_SOURCE_DIR}/locales/templates/pcsx2_Main.pot     ALL ${PO_MAIN_FILES})
    GETTEXT_CREATE_TRANSLATIONS_PCSX2(${PROJECT_SOURCE_DIR}/locales/templates/pcsx2_Tertiary.pot ALL ${PO_TER_FILES})

endif (GETTEXT_FOUND)
