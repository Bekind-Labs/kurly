# Re-runs lupdate on a copy of each .ts file and fails if that produces any
# untranslated ("unfinished") entries, which means the code has UI strings the
# translation doesn't cover yet.
file(GLOB ts_files "${SOURCE_DIR}/translations/*.ts")
file(GLOB sources "${SOURCE_DIR}/src/*.cpp" "${SOURCE_DIR}/src/*.h")
file(MAKE_DIRECTORY "${WORK_DIR}")

set(failed FALSE)
foreach(ts IN LISTS ts_files)
    get_filename_component(name "${ts}" NAME)
    set(copy "${WORK_DIR}/${name}")
    file(COPY_FILE "${ts}" "${copy}")
    execute_process(
        COMMAND "${LUPDATE}" -silent -no-obsolete -locations none ${sources} -ts "${copy}"
        RESULT_VARIABLE rc)
    if(NOT rc EQUAL 0)
        message(SEND_ERROR "lupdate failed for ${name}")
        set(failed TRUE)
        continue()
    endif()
    file(READ "${copy}" content)
    string(REGEX MATCHALL "<source>[^<]*</source>[^<]*(<comment>[^<]*</comment>[^<]*)?<translation type=\"unfinished\"" missing "${content}")
    list(LENGTH missing count)
    if(count GREATER 0)
        message(SEND_ERROR "${name}: ${count} string(s) need translating. Run "
                           "`cmake --build build --target update_translations` and translate them:\n${missing}")
        set(failed TRUE)
    endif()
endforeach()
if(failed)
    message(FATAL_ERROR "Translations are out of date.")
endif()
message(STATUS "All translations are up to date.")
