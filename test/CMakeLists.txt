link_libraries(asaCoder)


add_executable(getHello getHello.cpp)

target_link_libraries(getHello ${Boost_LIBRARIES})