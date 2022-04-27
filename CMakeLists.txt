cmake_minimum_required(VERSION 3.16)

#set project name and version
project (mw VERSION 0.9.2)

# Configure the project for testing with CTest/CDash
include(CTest)

# Configure generators for binary installers and source packages.
INCLUDE(CPack)

# Finds the pkg-config executable and adds the pkg_get_variable(), pkg_check_modules() and pkg_search_module() commands.
include(FindPkgConfig)

# C++ 17 required to build this target
set(CMAKE_CXX_STANDARD 17)

# Boolean describing whether the value of CXX_STANDARD is a requirement
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

# Build specific configuration
set(CMAKE_CXX_FLAGS_DEBUG_INIT "-Wall -Wextra -Werror -g3")
set(CMAKE_CXX_FLAGS_RELEASE_INIT "-Wall -O3")

# Find packages and load its package-specific details.
find_package(spdlog REQUIRED)
find_package(cpprestsdk REQUIRED)
find_package(PkgConfig REQUIRED)

# Checks for all the given modules, setting a variety of result variables in the calling scope
# The IMPORTED_TARGET argument creates an imported target named PkgConfig::<prefix> that can be passed directly as an argument to target_link_libraries()
pkg_check_modules(GMIME REQUIRED IMPORTED_TARGET gmime-3.0)
pkg_check_modules(TINYXML REQUIRED IMPORTED_TARGET tinyxml2)

# Adds a subdirectory to the build. The source_dir specifies the directory in which the source CMakeLists.txt and code files are located.
add_subdirectory(lib)

# Add directories that the compiler uses to search for include files.
include_directories(
    "${PROJECT_BINARY_DIR}"
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src
    SYSTEM
    ${SPDLOG_INCLUDEDIR}
    PkgConfig::GMIME
    PkgConfig::TINYXML
    )

# configure a header file to pass some of the CMake settings to the source code
configure_file("include/Version.h.in" "Version.h")

# Default value for <LANG>_CLANG_TIDY target property when <LANG> is C, CXX, OBJC or OBJCXX.
set(CMAKE_CXX_CLANG_TIDY clang-tidy --format-style=google --checks=clang-diagnostic-*,clang-analyzer-*,-*,bugprone*,modernize*,performance*)

# Adds an executable target called mw to be built from the source files listed in the command invocation
add_executable(mw src/main.cpp src/RpRestClient.cpp src/Service.cpp src/RestHandler.cpp src/Middleware.cpp)

# Specify libraries or flags to use when linking a given target and/or its dependents
target_link_libraries( mw
    LINK_PUBLIC
    spdlog::spdlog
    config++
    cpprestsdk::cpprest
    flute
    z
    PkgConfig::GMIME
    PkgConfig::TINYXML
)

# Generates installation rules for the project
install(TARGETS mw)

# Move supporting files to specific folders
install(FILES supporting_files/5gmag-rt-mw.service DESTINATION /usr/lib/systemd/system)
install(FILES supporting_files/5gmag-rt.conf DESTINATION /etc)
install(FILES supporting_files/5gmag-rt-wui DESTINATION /etc/nginx/sites-available/)

# Configure generators for binary installers and source packages.
SET(CPACK_GENERATOR "DEB")
SET(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "5G-MAG Reference Tools <reference-tools@5g-mag.com>")
SET(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_SOURCE_DIR}/debian/conffiles")