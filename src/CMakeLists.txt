cmake_minimum_required(VERSION 3.8 FATAL_ERROR)

add_definitions(-Wall -O2 -ldl)

find_package(Qt5Core REQUIRED)
find_package(Threads)

set(SOURCE_LIB
    sqlite3.c
    qsstatement.cpp
    qsconnection.cpp
    qsconnectionconfig.cpp
    qshelper.h
    qsconnectionworker.cpp
    qsconnectionasyncworker.cpp)

add_library(${PROJECT_NAME} STATIC ${SOURCE_LIB})

target_link_libraries(${PROJECT_NAME} ${CMAKE_DL_LIBS})
target_link_libraries(${PROJECT_NAME} Threads::Threads ${CMAKE_DL_LIBS})
target_link_libraries(${PROJECT_NAME} Qt5::Core)
