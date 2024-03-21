cmake_minimum_required(VERSION 3.24)
project(HMI-cmd LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# include_directories(BEFORE )

add_library(asaCoder asaCoder/asaEncoder.cpp)
# target_include_directories(asaCoder PUBLIC asaCoder)
# add_executable(HMI-snput)