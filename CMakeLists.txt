cmake_minimum_required(VERSION 3.16)

project (gw VERSION 0.9.0)

include(CTest)
include(FindPkgConfig)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_CXX_FLAGS_DEBUG_INIT "-Wall -Wextra -Werror -g3")
set(CMAKE_CXX_FLAGS_RELEASE_INIT "-Wall -O3")

find_package(Boost REQUIRED)
find_package(spdlog REQUIRED)
find_package(cpprestsdk REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GMIME REQUIRED IMPORTED_TARGET gmime-3.0)
pkg_check_modules(TINYXML REQUIRED IMPORTED_TARGET tinyxml2)

add_subdirectory(lib)

include_directories(
    "${PROJECT_BINARY_DIR}"
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/lib/mad_fcl_v1.7_src

    SYSTEM
    ${SPDLOG_INCLUDEDIR}
    PkgConfig::GMIME
    PkgConfig::TINYXML
    )

configure_file("include/Version.h.in" "Version.h")

link_directories(
    )

set(CMAKE_CXX_CLANG_TIDY clang-tidy --format-style=google --checks=clang-diagnostic-*,clang-analyzer-*,-*,bugprone*,modernize*,performance*)

add_executable(gw src/main.cpp src/FluteReceiver.cpp src/RpRestClient.cpp src/Service.cpp src/RestHandler.cpp)

target_link_libraries( gw
    LINK_PUBLIC
    spdlog::spdlog
    config++
    cpprestsdk::cpprest
    flutelib
    z
    pthread
    expat
    PkgConfig::GMIME
    PkgConfig::TINYXML
)


install(TARGETS gw DESTINATION bin LIBRARY DESTINATION lib)

SET(CPACK_GENERATOR "DEB")
SET(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "Austrian Broadcasting Services <obeca@ors.at>")
SET(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_SOURCE_DIR}/debian/conffiles;${CMAKE_SOURCE_DIR}/debian/postinst;${CMAKE_SOURCE_DIR}/debian/rp.service;")

INCLUDE(CPack)
