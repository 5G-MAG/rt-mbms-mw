cmake_minimum_required(VERSION 3.16)

project (libflute VERSION 0.9.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_CXX_FLAGS_DEBUG_INIT "-Wall -Wextra -Werror -g3")
set(CMAKE_CXX_FLAGS_RELEASE_INIT "-Wall -O3")

find_package(Boost REQUIRED)
find_package(spdlog REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(TINYXML REQUIRED IMPORTED_TARGET tinyxml2)

include_directories(
    "${PROJECT_BINARY_DIR}"
    ${PROJECT_SOURCE_DIR}/src

    SYSTEM
    ${SPDLOG_INCLUDEDIR}
    PkgConfig::TINYXML
    )

configure_file("include/Version.h.in" "Version.h")

link_directories(
    )

set(CMAKE_CXX_CLANG_TIDY clang-tidy --format-style=google --checks=clang-diagnostic-*,clang-analyzer-*,-*,bugprone*,modernize*,performance*)

add_library(flute "")
target_sources(flute
  PRIVATE
    src/Receiver.cpp src/Receiver.h src/AlcPacket.cpp src/File.cpp src/EncodingSymbol.cpp src/FileDeliveryTable.cpp
  PUBLIC
    src/Receiver.h src/File.h 
  )
target_include_directories(flute
  PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/src/
  )

#add_library(flute src/Receiver.cpp src/Receiver.h src/AlcPacket.cpp src/File.cpp src/EncodingSymbol.cpp src/FileDeliveryTable.cpp)

target_link_libraries( flute
    LINK_PUBLIC
    spdlog::spdlog
    z
    pthread
    PkgConfig::TINYXML
)

