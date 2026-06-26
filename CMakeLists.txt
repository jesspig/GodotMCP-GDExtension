cmake_minimum_required(VERSION 3.25)
project(GodotMCP LANGUAGES CXX C)

# Default Debug for single-config generators
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "" FORCE)
endif()

# Project version — single source of truth
set(PROJECT_VERSION "0.2.2-dev4")
message(STATUS "GodotMCP version: ${PROJECT_VERSION}")

# Compiler cache (sccache/ccache)
include(cmake/cache.cmake)

# LTO (Link-Time Optimization) — Release only
include(cmake/lto.cmake)

# GDExtension library
add_subdirectory(extensions)

# Post-build copy + CPack
include(cmake/copy-gdext.cmake)

# Deep clean (guarded: requires GODOTMCP_ALLOW_DEEP_CLEAN=yes env to proceed)
add_custom_target(deep-clean
    COMMAND ${CMAKE_COMMAND}
        -D_GDEXT_BIN_DIR="${_gdext_bin_dir}"
        -D_DEPS_DIR="${CMAKE_CURRENT_BINARY_DIR}/_deps"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/deep-clean.cmake"
    COMMENT "Cleaning addons/bin/ and FetchContent cache"
)
