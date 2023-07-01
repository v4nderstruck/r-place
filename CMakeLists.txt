cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 17)

project(rplace)

include(ExternalProject)
include(FetchContent)

FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz)
FetchContent_MakeAvailable(json)

ExternalProject_Add(ACTOR
  PREFIX DEPS
  SOURCE_DIR ${CMAKE_SOURCE_DIR}/third-party/actor-framework
  UPDATE_COMMAND ""  
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/INSTALL -DCMAKE_OSX_ARCHITECTURES=arm64
)

link_directories(${CMAKE_CURRENT_BINARY_DIR}/INSTALL/lib)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/INSTALL/include)

# Add your main project
add_executable(rplace src/main.cpp)
target_link_libraries(rplace PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(rplace PUBLIC 
  libcaf_core.dylib 
  libcaf_net.dylib 
  libcaf_io.dylib
  libcaf_openssl.dylib
)
