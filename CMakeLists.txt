cmake_minimum_required(VERSION 3.24)

project(HMI-cmd LANGUAGES CXX)

include(CMakePrintHelpers)

cmake_print_variables(CMAKE_SYSTEM_NAME) # 操作系統
cmake_print_variables(CMAKE_SYSTEM_VERSION) # 操作系統版本
cmake_print_variables(CMAKE_CXX_COMPILER_ID) # 編譯器
cmake_print_variables(CMAKE_CXX_COMPILER_VERSION) # 編譯器版本

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

set(BOOST_ROOT "${CMAKE_SOURCE_DIR}/boost_1_85_0/stage/lib/cmake")
set(Boost_LIBRARY_DIRS boost_1_85_0)
option(Boost_USE_STATIC_LIBS "" ON)
option(Boost_USE_MULTITHREADED "" ON)
option(Boost_USE_STATIC_RUNTIME "" ON)
option(USE_CXX_STD_20 "" ON)
set(Boost_ARCHITECTURE "-x64")
find_package(Boost COMPONENTS date_time system program_options REQUIRED)
cmake_print_variables(${Boost_INCLUDE_DIRS})
include_directories(${Boost_INCLUDE_DIRS})
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# add_compile_definitions(_GLIBCXX_USE_CXX11_ABI=0)
# include_directories(ncurses/include)
# link_directories(ncurses/lib)
include_directories(asaCoder)
# include_directories(cmd)
add_subdirectory(cmd)

add_subdirectory(cxxopts-3.2.0)

add_library(asaCoder 
    asaCoder/asaEncoder.cpp
    asaCoder/serialPort.cpp
    # cmd/HMI.cpp
    )

# include_directories(${CMAKE_SOURCE_DIR}/ncurses/include)
# link_directories(${CMAKE_SOURCE_DIR}/ncurses/lib)
# add_definitions(-DNCURSES_STATIC)
# add_compile_options("-fno-stack-protector")

# add_executable(PCDataAgent cmd/cmdAgent.cpp)
# target_link_libraries(PCDataAgent ncursesw -static)
# target_include_directories(PCDataAgent INTERFACE ${CMAKE_SOURCE_DIR}/ncurses/include)
# target_link_directories(PCDataAgent INTERFACE ${CMAKE_SOURCE_DIR}/ncurses/lib)

# target_link_libraries(asaCoder Boost::asio)
target_link_libraries(asaCoder Threads::Threads)
if(WIN32)
    target_link_libraries(asaCoder wsock32 ws2_32)
endif()
# target_link_libraries(asaCoder 
#     advapi32
#     setupapi
#     )
add_subdirectory(test)

# target_include_directories(asaCoder PUBLIC asaCoder)
# add_executable(HMI-snput)