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