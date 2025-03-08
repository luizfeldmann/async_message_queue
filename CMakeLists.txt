#
# Copyright (c) 2025 Luiz Feldmann
#

CMAKE_MINIMUM_REQUIRED(VERSION 3.22)

# Read project version from file
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/package.json" PACKAGE_JSON)
string(JSON CMAKE_PROJECT_NAME GET "${PACKAGE_JSON}" "name")
string(JSON CMAKE_PROJECT_VERSION GET "${PACKAGE_JSON}" "version")
string(JSON CMAKE_PROJECT_HOMEPAGE_URL GET "${PACKAGE_JSON}" "homepage")
string(JSON CMAKE_PROJECT_DESCRIPTION GET "${PACKAGE_JSON}" "description")

PROJECT(${CMAKE_PROJECT_NAME}
    LANGUAGES CXX
    VERSION ${CMAKE_PROJECT_VERSION}
    HOMEPAGE_URL "${CMAKE_PROJECT_HOMEPAGE_URL}"
    DESCRIPTION "${CMAKE_PROJECT_DESCRIPTION}")

# Link to find modules
list(APPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/build/generators")

# Default directories
include(GNUInstallDirs)

# Add library
set(PUBLIC_HEADERS
    "include/async_message_queue/async_message_queue.hpp"
    )

add_library(${CMAKE_PROJECT_NAME} INTERFACE
    ${PUBLIC_HEADERS})

find_package(Boost REQUIRED)
target_link_libraries(${CMAKE_PROJECT_NAME} INTERFACE
    Boost::boost )

IF (MSVC)
    target_compile_definitions(${CMAKE_PROJECT_NAME} INTERFACE _WIN32_WINNT=0x0601)
ENDIF()

target_include_directories(${CMAKE_PROJECT_NAME} INTERFACE
    "include" )

target_sources(${CMAKE_PROJECT_NAME} 
    PUBLIC FILE_SET public_headers
    TYPE HEADERS BASE_DIRS "include"
    FILES ${PUBLIC_HEADERS} )


# Make a findable package
install(TARGETS ${CMAKE_PROJECT_NAME}
    INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    FILE_SET public_headers DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

# Tests
if (NOT BUILD_TESTING STREQUAL OFF)
    enable_testing()
    add_subdirectory(tests)
endif()
