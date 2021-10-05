cmake_minimum_required(VERSION 3.16)

project (mw VERSION 0.9.0)

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
pkg_check_modules(GMIME REQUIRED IMPORTED_TARGET gmime-3.0)
pkg_check_modules(TINYXML REQUIRED IMPORTED_TARGET tinyxml2)

add_subdirectory(lib)

include_directories(
    "${PROJECT_BINARY_DIR}"
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src

    SYSTEM
    ${SPDLOG_INCLUDEDIR}
    PkgConfig::GMIME
    PkgConfig::TINYXML
    )

configure_file("include/Version.h.in" "Version.h")

link_directories(
    )

set(CMAKE_CXX_CLANG_TIDY clang-tidy --format-style=google --checks=clang-diagnostic-*,clang-analyzer-*,-*,bugprone*,modernize*,performance*)

add_executable(mw src/main.cpp src/RpRestClient.cpp src/Service.cpp src/RestHandler.cpp src/Middleware.cpp)

target_link_libraries( mw
    LINK_PUBLIC
    spdlog::spdlog
    config++
    cpprestsdk::cpprest
    flute
    z
    pthread
    expat
    PkgConfig::GMIME
    PkgConfig::TINYXML
)


install(TARGETS mw)
install(FILES supporting_files/5gmag-rt-mw.service DESTINATION /usr/lib/systemd/system)
install(FILES supporting_files/5gmag-rt.conf DESTINATION /etc)

SET(CPACK_GENERATOR "DEB")
SET(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "5G-MAG Reference Tools <reference-tools@5g-mag.com>")
SET(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_SOURCE_DIR}/debian/conffiles")

INCLUDE(CPack)
