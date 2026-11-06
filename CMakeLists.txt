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

qt_add_executable(kurly
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
)
target_link_libraries(kurly PRIVATE kurly_core Qt6::Widgets)
set_target_properties(kurly PROPERTIES WIN32_EXECUTABLE ON MACOSX_BUNDLE ON)

# Source strings are English; Japanese is provided as a translation.
# The compiled .qm files are embedded into the executable under :/i18n.
qt_add_translations(kurly TS_FILES translations/kurly_ja.ts)

enable_testing()
qt_add_executable(tst_curlcommand tests/tst_curlcommand.cpp)
target_link_libraries(tst_curlcommand PRIVATE kurly_core Qt6::Test)
add_test(NAME curlcommand COMMAND tst_curlcommand)
