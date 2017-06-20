cmake_minimum_required(VERSION 3.15)

project(globaltop-gps LANGUAGES C CXX)

find_package(Qt6 COMPONENTS Core SerialPort REQUIRED)

add_executable(globaltop-gps
    main.cpp
    gps.c
    gps.h
)

target_link_libraries(globaltop-gps
    Qt6::Core
    Qt6::SerialPort
)
