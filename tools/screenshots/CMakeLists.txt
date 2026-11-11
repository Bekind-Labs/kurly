# Takes the website's product screenshots; see main.cpp.
qt_add_executable(kurly-screenshots main.cpp)
target_link_libraries(kurly-screenshots PRIVATE kurly_gui)
