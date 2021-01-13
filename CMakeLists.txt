cmake_minimum_required(VERSION 3.1)
project(ibl VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)

## SOURCE FILES

file(GLOB_RECURSE SOURCE_FILES
        src/*.c
        src/*.cpp)

## THIRD PARTY

include_directories(third_party/include/)

file(GLOB_RECURSE LIB_SOURCE_FILES
        third_party/src/*.c
        third_party/src/*.cpp)

set(SOURCE_FILES ${SOURCE_FILES} ${LIB_SOURCE_FILES})

#GLFW
find_package(glfw3 REQUIRED)

#OPENGl
find_package(OpenGL REQUIRED)


set(LIBS glfw)

add_executable(ibl ${SOURCE_FILES})
target_link_libraries(ibl ${LIBS})