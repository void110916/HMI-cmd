link_libraries(asaCoder)

add_executable(hello hello.cpp)
target_link_libraries(hello HMI)
target_link_libraries(hello wsock32 ws2_32)

add_executable(getHello getHello.cpp)
target_link_libraries(getHello HMI)
target_link_libraries(getHello wsock32 ws2_32)

add_executable(putArray putArray.cpp)
target_link_libraries(putArray HMI)
target_link_libraries(putArray wsock32 ws2_32)

add_executable(sngetArray sngetArray.cpp)
target_link_libraries(sngetArray HMI)
target_link_libraries(sngetArray wsock32 ws2_32)

include_directories(${CMAKE_SOURCE_DIR}/ncurses/include)
link_directories(${CMAKE_SOURCE_DIR}/ncurses/lib)
add_definitions(-DNCURSES_STATIC)
add_compile_options("-fno-stack-protector")

add_executable(windowBox windowBox.cpp)
target_link_libraries(windowBox ncursesw -static)
target_include_directories(windowBox PUBLIC ${CMAKE_SOURCE_DIR}/ncurses/include)
target_link_directories(windowBox PUBLIC ${CMAKE_SOURCE_DIR}/ncurses/lib)

add_executable(serial2win serial2win.cpp)
target_link_libraries(serial2win asaCoder)
target_link_libraries(serial2win ncursesw -static)
target_include_directories(serial2win PUBLIC ${CMAKE_SOURCE_DIR}/ncurses/include)
target_link_directories(serial2win PUBLIC ${CMAKE_SOURCE_DIR}/ncurses/lib)