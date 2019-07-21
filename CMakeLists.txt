CMAKE_MINIMUM_REQUIRED(VERSION 3.1)
PROJECT(stencil)

SET(CMAKE_CXX_STANDARD 14)

SET(SOURCES
    src/buffer.hpp
    src/loop.hpp
    src/util.hpp)

SET(DEMO_HEAT_DISSIPATION_SOURCES
    demo/heat_dissipation_2d.cpp)

SET(TEST_SOURCES
    test/buffer.cpp
    test/main.cpp
    test/util.cpp)

INCLUDE_DIRECTORIES(src dep dep/catch/single_include)

ADD_EXECUTABLE(run_tests ${SOURCES} ${TEST_SOURCES})

FIND_PACKAGE(SDL2 REQUIRED)

# FIXME: this should only be required for the demos, but none of the single-target commands work
SET(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -fopenmp)

ADD_EXECUTABLE(demo_heat_dissipation ${DEMO_HEAT_DISSIPATION_SOURCES})
INCLUDE_DIRECTORIES(demo_heat_dissipation ${SDL2_INCLUDE_DIRS})
TARGET_LINK_LIBRARIES(demo_heat_dissipation ${SDL2_LIBRARIES})

ADD_CUSTOM_TARGET(format
    COMMAND clang-format -style=file -i ${SOURCES} ${TEST_SOURCES} ${DEMO_HEAT_DISSIPATION_SOURCES}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

