# Qt Network provides the local HTTP server the run tests talk to.
find_package(Qt6 REQUIRED COMPONENTS Network)

# Each test is a standalone QtTest executable registered with CTest.
function(kurly_add_test name)
    cmake_parse_arguments(ARG "GUI" "" "LIBS" ${ARGN})
    qt_add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE ${ARG_LIBS} Qt6::Test)
    add_test(NAME ${name} COMMAND ${name})
    if(ARG_GUI)
        # Run widget tests without opening windows or needing a display.
        set_tests_properties(${name} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    endif()
endfunction()

kurly_add_test(tst_curlcommand LIBS kurly_core)
kurly_add_test(tst_curlparser LIBS kurly_core)
kurly_add_test(tst_responseformatter LIBS kurly_core)
kurly_add_test(tst_requeststore LIBS kurly_core)
kurly_add_test(tst_requestrunner LIBS kurly_core Qt6::Network)
kurly_add_test(tst_importdialog GUI LIBS kurly_gui)
kurly_add_test(tst_mainwindow GUI LIBS kurly_gui Qt6::Network)
kurly_add_test(tst_library GUI LIBS kurly_gui Qt6::Network)

# Checks the shipped Japanese translation for completeness and consistency.
kurly_add_test(tst_translations LIBS Qt6::Core)
target_compile_definitions(tst_translations PRIVATE
    KURLY_TS_FILE="${PROJECT_SOURCE_DIR}/translations/kurly_ja.ts")

# Fails if UI strings were added or changed in the code without updating the
# .ts file (i.e. someone forgot `cmake --build build --target update_translations`).
add_test(NAME translations_up_to_date
    COMMAND ${CMAKE_COMMAND}
        -DLUPDATE=$<TARGET_FILE:Qt6::lupdate>
        -DSOURCE_DIR=${PROJECT_SOURCE_DIR}
        -DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/ts-check
        -P ${CMAKE_CURRENT_SOURCE_DIR}/check_translations.cmake)
