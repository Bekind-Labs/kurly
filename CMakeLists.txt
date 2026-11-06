cmake_minimum_required(VERSION 3.21)
project(kurly VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.2 REQUIRED COMPONENTS Widgets LinguistTools Test)

# UI-independent logic, shared by the app and the tests.
add_library(kurly_core STATIC src/curlcommand.cpp src/curlcommand.h)
target_include_directories(kurly_core PUBLIC src)
target_link_libraries(kurly_core PUBLIC Qt6::Core)

set(KURLY_ICON_SIZES 16 32 48 64 128 256 512)
set(KURLY_ICON_PNGS)
foreach(size IN LISTS KURLY_ICON_SIZES)
    list(APPEND KURLY_ICON_PNGS icons/png/kurly-${size}.png)
endforeach()

qt_add_executable(kurly
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    src/spacing.h
)
target_link_libraries(kurly PRIVATE kurly_core Qt6::Widgets)

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

# Source strings are English; Japanese is provided as a translation.
# The compiled .qm files are embedded into the executable under :/i18n.
qt_add_translations(kurly TS_FILES translations/kurly_ja.ts)

enable_testing()
qt_add_executable(tst_curlcommand tests/tst_curlcommand.cpp)
target_link_libraries(tst_curlcommand PRIVATE kurly_core Qt6::Test)
add_test(NAME curlcommand COMMAND tst_curlcommand)
