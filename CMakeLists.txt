cmake_minimum_required(VERSION 3.8 FATAL_ERROR)

find_package(Threads)
find_package(Qt5 COMPONENTS Core REQUIRED)

option(BUILD_SHARED_LIBS "Build as shared libraries" ON)

add_library(QsSqlite "")

target_compile_features(QsSqlite PRIVATE cxx_std_14)

set_target_properties(QsSqlite PROPERTIES
    CXX_EXTENSIONS OFF
    THREADS_PREFER_PTHREAD_FLAG ON
    AUTOMOC ON)

target_sources(QsSqlite
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include/sqlite3.h
        ${CMAKE_CURRENT_LIST_DIR}/include/qsstatement.h
        ${CMAKE_CURRENT_LIST_DIR}/include/qsconnection.h
        ${CMAKE_CURRENT_LIST_DIR}/include/qsconnectionconfig.h
        ${CMAKE_CURRENT_LIST_DIR}/include/qsconnectionworker.h
        ${CMAKE_CURRENT_LIST_DIR}/include/qsconnectionasyncworker.h
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src/sqlite3.c
        ${CMAKE_CURRENT_LIST_DIR}/src/qsstatement.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/qsconnection.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/qshelper.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/qshelper.h
        ${CMAKE_CURRENT_LIST_DIR}/src/qsconnectionconfig.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/qsconnectionworker.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/qsconnectionasyncworker.cpp)

target_link_libraries(QsSqlite ${CMAKE_DL_LIBS})
target_link_libraries(QsSqlite Threads::Threads)
target_link_libraries(QsSqlite Qt5::Core)
