include_directories(${Boost_INCLUDE_DIRS})
link_libraries(asaCoder ${Boost_LIBRARIES})



add_library(HMI HMI.cpp)
target_include_directories(HMI PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
# target_link_libraries(HMI ${Boost_LIBRARIES} asaCoder)
# target_link_libraries(HMI wsock32 ws2_32)
# add_compile_definitions(_GLIBCXX_USE_CXX11_ABI=0)
# add_executable(cmdAgent cmdAgent.cpp)
# target_link_libraries(cmdAgent HMI cxxopts)
# target_include_directories(cmdAgent INTERFACE cxxopts)


include_directories(${CMAKE_SOURCE_DIR}/ncurses/include)
link_directories(${CMAKE_SOURCE_DIR}/ncurses/lib)
add_definitions(-DNCURSES_STATIC)
add_compile_options("-fno-stack-protector")

add_executable(PCDataAgent cmdAgent.cpp)
target_link_libraries(PCDataAgent HMI cxxopts)
target_link_libraries(PCDataAgent ncursesw -static)
target_include_directories(PCDataAgent PUBLIC ${CMAKE_SOURCE_DIR}/ncurses/include)
target_link_directories(PCDataAgent PUBLIC ${CMAKE_SOURCE_DIR}/ncurses/lib)

