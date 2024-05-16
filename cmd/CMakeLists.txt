include_directories(${Boost_INCLUDE_DIRS})
link_libraries(asaCoder ${Boost_LIBRARIES})


add_library(HMI HMI.cpp)
target_include_directories(HMI PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
# target_link_libraries(HMI ${Boost_LIBRARIES} asaCoder)
# target_link_libraries(HMI wsock32 ws2_32)
# add_compile_definitions(_GLIBCXX_USE_CXX11_ABI=0)
add_executable(cmdAgent cmdAgent.cpp)
target_link_libraries(cmdAgent HMI cxxopts)
target_include_directories(cmdAgent INTERFACE cxxopts)
