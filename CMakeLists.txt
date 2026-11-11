cmake_minimum_required(VERSION 3.21)
project(kurly VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# Sources contain UTF-8 (Japanese) literals; MSVC otherwise assumes the
# system code page and garbles them.
if(MSVC)
    add_compile_options(/utf-8)
endif()

find_package(Qt6 6.2 REQUIRED COMPONENTS Widgets LinguistTools Test)

option(KURLY_BUILD_TESTS "Build the unit tests" ON)
option(KURLY_COVERAGE "Instrument kurly_core/kurly_gui for code coverage (GCC/Clang)" OFF)
option(KURLY_BUILD_SCREENSHOT_TOOL "Build the tool that takes the website's screenshots" OFF)
option(KURLY_REQUIRE_QT_TRANSLATIONS
       "Fail if Qt's own Japanese translation (qtbase_ja.qm) can't be found" OFF)

# UI-independent logic: building and parsing curl command lines.
add_library(kurly_core STATIC
    src/curlcommand.cpp src/curlcommand.h
    src/curlparser.cpp src/curlparser.h
    src/responseformatter.cpp src/responseformatter.h
    src/requeststore.cpp src/requeststore.h
    src/requestrunner.cpp src/requestrunner.h
)
target_include_directories(kurly_core PUBLIC src)
target_link_libraries(kurly_core PUBLIC Qt6::Core)

# The window and dialogs, in a library so tests can drive them directly.
add_library(kurly_gui STATIC
    src/mainwindow.cpp src/mainwindow.h
    src/importdialog.cpp src/importdialog.h
    src/librarypanel.cpp src/librarypanel.h
    src/spacing.h
    src/themecolors.h
)
target_link_libraries(kurly_gui PUBLIC kurly_core Qt6::Widgets)

# Source strings are English; Japanese is provided as a translation.
# The compiled .qm files are embedded under :/i18n.
qt_add_translations(kurly_gui TS_FILES translations/kurly_ja.ts)

# Qt's own strings (standard buttons, text field context menus, ...) come from
# Qt's translations. Embed the Japanese one so packaged apps carry it too.
set(qt_translation_dirs "${QT6_INSTALL_PREFIX}/${QT6_INSTALL_TRANSLATIONS}")
foreach(prefix IN LISTS CMAKE_PREFIX_PATH)
    list(APPEND qt_translation_dirs "${prefix}/translations"
         "${prefix}/share/qt/translations" "${prefix}/share/qt6/translations")
endforeach()
find_file(KURLY_QTBASE_JA_QM qtbase_ja.qm
    HINTS ${qt_translation_dirs}
    PATHS /usr/share/qt6/translations /usr/share/qt/translations
    NO_DEFAULT_PATH)
if(KURLY_QTBASE_JA_QM)
    message(STATUS "Embedding Qt's Japanese translation: ${KURLY_QTBASE_JA_QM}")
    set_source_files_properties(${KURLY_QTBASE_JA_QM} PROPERTIES QT_RESOURCE_ALIAS qtbase_ja.qm)
    qt_add_resources(kurly_gui "qt_translations" PREFIX "/i18n" FILES ${KURLY_QTBASE_JA_QM})
elseif(KURLY_REQUIRE_QT_TRANSLATIONS)
    message(FATAL_ERROR "qtbase_ja.qm not found. Install Qt's translations "
                        "(e.g. qt6-translations-l10n on Debian/Ubuntu, qt6-translations on Arch).")
else()
    message(WARNING "qtbase_ja.qm not found: Qt's built-in texts (OK buttons, context menus) "
                    "will stay in English in Japanese mode.")
endif()

if(KURLY_COVERAGE)
    foreach(target kurly_core kurly_gui)
        target_compile_options(${target} PRIVATE --coverage -O0 -g)
        target_link_options(${target} PUBLIC --coverage)
    endforeach()
endif()

set(KURLY_ICON_SIZES 16 32 48 64 128 256 512)
set(KURLY_ICON_PNGS)
foreach(size IN LISTS KURLY_ICON_SIZES)
    list(APPEND KURLY_ICON_PNGS icons/png/kurly-${size}.png)
endforeach()

qt_add_executable(kurly src/main.cpp)
target_link_libraries(kurly PRIVATE kurly_gui)
target_compile_definitions(kurly PRIVATE KURLY_VERSION="${PROJECT_VERSION}")

# Window/taskbar icon, embedded so it works on every platform.
qt_add_resources(kurly "icons"
    PREFIX "/"
    FILES ${KURLY_ICON_PNGS}
)

if(APPLE)
    # Finder/Dock icon for the .app bundle.
    set(KURLY_ICNS ${CMAKE_CURRENT_SOURCE_DIR}/icons/kurly.icns)
    target_sources(kurly PRIVATE ${KURLY_ICNS})
    set_source_files_properties(${KURLY_ICNS} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    set_target_properties(kurly PROPERTIES
        MACOSX_BUNDLE ON
        MACOSX_BUNDLE_BUNDLE_NAME Kurly
        MACOSX_BUNDLE_GUI_IDENTIFIER com.bekindlabs.kurly
        MACOSX_BUNDLE_ICON_FILE kurly.icns
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    )
elseif(WIN32)
    # Explorer/taskbar icon for kurly.exe.
    target_sources(kurly PRIVATE icons/kurly.rc)
    set_target_properties(kurly PROPERTIES WIN32_EXECUTABLE ON)
endif()

include(GNUInstallDirs)
install(TARGETS kurly
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
if(UNIX AND NOT APPLE)
    install(FILES LICENSE THIRD_PARTY_NOTICES.md licenses/LGPL-3.0.txt licenses/GPL-3.0.txt
        DESTINATION ${CMAKE_INSTALL_DOCDIR})
    # Desktop launcher and themed icons for Linux desktops.
    install(FILES icons/kurly.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
    install(FILES icons/kurly.svg
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
    foreach(size IN LISTS KURLY_ICON_SIZES)
        install(FILES icons/png/kurly-${size}.png
            DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/${size}x${size}/apps
            RENAME kurly.png)
    endforeach()
endif()

if(KURLY_BUILD_SCREENSHOT_TOOL)
    add_subdirectory(tools/screenshots)
endif()

if(KURLY_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
