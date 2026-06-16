cmake_minimum_required(VERSION 3.25)
project(GodotMCP LANGUAGES CXX C)

# ---------------------------------------------------------------------------
# Options
# ---------------------------------------------------------------------------
# Default to Debug when single-config generator and nothing was requested.
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "" FORCE)
endif()

# ---------------------------------------------------------------------------
# Project version — maintained directly here (no more Cargo.toml parsing).
# Bump when releasing.
# ---------------------------------------------------------------------------
set(PROJECT_VERSION "0.2.1-dev2")
message(STATUS "GodotMCP version: ${PROJECT_VERSION}")

# ---------------------------------------------------------------------------
# Compiler launcher (sccache/ccache) — speeds up incremental builds 2-5x.
# Installed via: winget install sccache  |  choco install sccache
# ---------------------------------------------------------------------------
find_program(GODOTMCP_CXX_LAUNCHER NAMES sccache ccache)
if(GODOTMCP_CXX_LAUNCHER)
    message(STATUS "Using compiler launcher: ${GODOTMCP_CXX_LAUNCHER}")
    # Propagate to all targets (including godot-cpp via FetchContent)
    set(CMAKE_C_COMPILER_LAUNCHER "${GODOTMCP_CXX_LAUNCHER}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${GODOTMCP_CXX_LAUNCHER}")
endif()

# ---------------------------------------------------------------------------
# C++ GDExtension (godot-cpp 10.0.0-rc1 via FetchContent)
# ---------------------------------------------------------------------------

# MSVC: treat source files & execution charset as UTF-8 (fixes CJK in string literals)
if(MSVC)
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT Embedded)
endif()

# ---------------------------------------------------------------------------
# LTO (Link-Time Optimization) — only for Release builds
# ---------------------------------------------------------------------------
include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_supported OUTPUT ipo_error)
if(ipo_supported)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
    message(STATUS "LTO enabled for Release builds")
else()
    message(STATUS "LTO not supported: ${ipo_error}")
endif()

add_subdirectory(extensions)

# ---------------------------------------------------------------------------
# Copy GDExtension library to addons/bin/ (post-build)
# ---------------------------------------------------------------------------
set(_addon_dir "${CMAKE_SOURCE_DIR}/example/addons/godot_mcp")
set(_gdext_bin_dir "${_addon_dir}/bin")
if(WIN32)
    set(_gdext_lib_name "godot_mcp_gdext.dll")
elseif(APPLE)
    set(_gdext_lib_name "libgodot_mcp_gdext.dylib")
else()
    set(_gdext_lib_name "libgodot_mcp_gdext.so")
endif()

add_custom_target(copy-gdext ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:godot_mcp_gdext>"
        "${_gdext_bin_dir}/${_gdext_lib_name}"
    COMMENT "Copying GDExtension to addons/bin/"
)
add_dependencies(copy-gdext godot_mcp_gdext)

# ---------------------------------------------------------------------------
# Install rules (for CPack packaging)
# ---------------------------------------------------------------------------
install(
    DIRECTORY "${CMAKE_SOURCE_DIR}/example/addons/"
    DESTINATION "example/addons"
    USE_SOURCE_PERMISSIONS
    PATTERN "bin/*.ilk" EXCLUDE
    PATTERN "bin/*.pdb" EXCLUDE
    PATTERN "bin/*.exp" EXCLUDE
    PATTERN "bin/*.lib" EXCLUDE
)

set(CPACK_GENERATOR "ZIP")
set(CPACK_PACKAGE_FILE_NAME "godot-mcp-${PROJECT_VERSION}")
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
set(CPACK_STRIP_FILES ON)
include(CPack)

# ---------------------------------------------------------------------------
# Utility targets
# ---------------------------------------------------------------------------
add_custom_target(deep-clean
    COMMAND ${CMAKE_COMMAND} -E rm -rf "${_gdext_bin_dir}"
    COMMAND ${CMAKE_COMMAND} -E rm -rf "${CMAKE_CURRENT_BINARY_DIR}/_deps"
    COMMENT "Cleaning addons/bin/ and FetchContent cache"
)
