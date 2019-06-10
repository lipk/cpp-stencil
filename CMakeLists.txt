CMAKE_MINIMUM_REQUIRED(VERSION 3.1)
PROJECT(stencil)

SET(CMAKE_CXX_STANDARD 11)

SET(SOURCES
    src/buffer.hpp
    src/loop.hpp
    src/util.hpp)

SET(TEST_SOURCES
    test/buffer.cpp
    test/main.cpp)

INCLUDE_DIRECTORIES(src dep dep/catch/single_include)

ADD_EXECUTABLE(run_tests ${SOURCES} ${TEST_SOURCES})

ADD_CUSTOM_TARGET(format
    COMMAND clang-format -style=file -i ${SOURCES} ${TEST_SOURCES}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
