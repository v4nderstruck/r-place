cmake_minimum_required(VERSION 3.10)
project(rplace)

set(CMAKE_CXX_STANDARD 11)

include(ExternalProject)
include(FetchContent)

FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz)
FetchContent_MakeAvailable(json)


set(INSTALL_DIR ${CMAKE_BINARY_DIR}/INSTALL)
set(THIRD_PARTY_DIR ${CMAKE_SOURCE_DIR}/third-party)

set(Boost_LIBRARY_DIR ${INSTALL_DIR}/lib/boost/)
set(Boost_INCLUDE_DIR ${INSTALL_DIR}/include/)

ExternalProject_Add(
  boost
  URL ${THIRD_PARTY_DIR}/boost_1_82_0.tar.gz
  SOURCE_DIR ${CMAKE_BINARY_DIR}/boost
  BUILD_IN_SOURCE 1
  CONFIGURE_COMMAND ./bootstrap.sh  --prefix=${INSTALL_DIR}
  BUILD_COMMAND ./b2 install -j8 --without-mpi
  INSTALL_DIR ${INSTALL_DIR}
)

ExternalProject_Add(
  plog 
  URL ${THIRD_PARTY_DIR}/plog-1.1.9.tar.gz
  SOURCE_DIR ${CMAKE_BINARY_DIR}/plog
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND cp -r ${CMAKE_BINARY_DIR}/plog/include/plog ${INSTALL_DIR}/include/
  INSTALL_DIR ${INSTALL_DIR}
)

ExternalProject_Add(
  flatbuffers
  GIT_REPOSITORY https://github.com/google/flatbuffers  
  SOURCE_DIR ${CMAKE_BINARY_DIR}/flatbuffers
  GIT_TAG v23.5.26
  CONFIGURE_COMMAND ""
  INSTALL_COMMAND cp -r ${CMAKE_BINARY_DIR}/flatbuffers/include/flatbuffers ${INSTALL_DIR}/include/
  BUILD_COMMAND ""
)

configure_file(${THIRD_PARTY_DIR}/CLI11.hpp ${INSTALL_DIR}/include/CLI11.hpp COPYONLY)
include_directories(${INSTALL_DIR}/include)

file(GLOB SRC
     "src/*.h"
     "src/*.cpp"
)

add_executable(rplace ${SRC})
target_link_libraries(rplace PRIVATE nlohmann_json::nlohmann_json)


