cmake_minimum_required(VERSION 3.24)

project(HMI-cmd LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(BOOST_ROOT "${CMAKE_SOURCE_DIR}/boost_1_85_0/stage/lib/cmake")
# set(Boost_LIBRARY_DIRS boost_1_85_0)
set(Boost_USE_STATIC_LIBS ON)
set(Boost_ARCHITECTURE "-x64")
find_package(Boost COMPONENTS date_time system REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

include_directories(asaCoder)

add_library(asaCoder 
    asaCoder/asaEncoder.cpp
    asaCoder/serialPort.cpp
    )
if(MINGW)
    target_link_libraries(asaCoder ws2_32)
endif()
# target_link_libraries(asaCoder 
#     advapi32
#     setupapi
#     )
add_subdirectory(test)

# target_include_directories(asaCoder PUBLIC asaCoder)
# add_executable(HMI-snput)