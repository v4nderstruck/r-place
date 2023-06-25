cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 17)

project(rplace)

include(ExternalProject)

ExternalProject_Add(ACTOR
  PREFIX DEPS
  SOURCE_DIR ${CMAKE_SOURCE_DIR}/third-party/actor-framework
  UPDATE_COMMAND ""  
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/INSTALL
)

link_directories(${CMAKE_CURRENT_BINARY_DIR}/INSTALL/lib)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/INSTALL/include)

# Add your main project
add_executable(rplace src/main.cpp)
target_link_libraries(rplace PUBLIC libcaf_core.dylib)
