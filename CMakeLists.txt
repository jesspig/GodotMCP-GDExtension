cmake_minimum_required(VERSION 3.25)
project(GodotMCP LANGUAGES CXX C)

# ---------------------------------------------------------------------------
# Options
# ---------------------------------------------------------------------------
option(RELEASE    "Build with Release configuration" OFF)

# Default to Debug when single-config generator and nothing was requested.
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    if(RELEASE)
        set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
    else()
        set(CMAKE_BUILD_TYPE "Debug"   CACHE STRING "" FORCE)
    endif()
endif()

# ---------------------------------------------------------------------------
# Project version — maintained directly here (no more Cargo.toml parsing).
# Bump when releasing.
# ---------------------------------------------------------------------------
set(PROJECT_VERSION "0.2.0")
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
    add_compile_options(/utf-8 /bigobj /D_CRT_SECURE_NO_WARNINGS)
    # Embed debug info in .obj files (/Z7) to avoid C1041 PDB conflicts with sccache + /MP.
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT Embedded)
endif()

add_subdirectory(extensions)

# ---------------------------------------------------------------------------
# Generate addon config files (plugin.cfg, .gdextension)
# ---------------------------------------------------------------------------
set(_addon_dir "${CMAKE_SOURCE_DIR}/example/addons/godot_mcp")
set(_gdext_bin_dir "${_addon_dir}/bin")
file(MAKE_DIRECTORY "${_addon_dir}")
file(MAKE_DIRECTORY "${_gdext_bin_dir}")

# ---------------------------------------------------------------------------
# Generate addon config files (plugin.cfg, .gdextension)
# ---------------------------------------------------------------------------
file(WRITE "${_addon_dir}/plugin.cfg"
    "[plugin]\n"
    "name=\"Godot MCP\"\n"
    "description=\"Model Context Protocol bridge for Godot Engine.\"\n"
    "author=\"JessPig\"\n"
    "version=\"${PROJECT_VERSION}\"\n"
    "script=\"\"\n"
)

file(WRITE "${_addon_dir}/godot_mcp.gdextension"
    "[configuration]\n"
    "\n"
    "entry_symbol = \"gdext_mcp_init\"\n"
    "compatibility_minimum = \"4.6\"\n"
    "reloadable = true\n"
    "\n"
    "[libraries]\n"
    "\n"
    "windows.debug.x86_64   = \"res://addons/godot_mcp/bin/godot_mcp_gdext.dll\"\n"
    "windows.release.x86_64 = \"res://addons/godot_mcp/bin/godot_mcp_gdext.dll\"\n"
    "linux.debug.x86_64     = \"res://addons/godot_mcp/bin/libgodot_mcp_gdext.so\"\n"
    "linux.release.x86_64   = \"res://addons/godot_mcp/bin/libgodot_mcp_gdext.so\"\n"
    "macos.debug            = \"res://addons/godot_mcp/bin/libgodot_mcp_gdext.dylib\"\n"
    "macos.release          = \"res://addons/godot_mcp/bin/libgodot_mcp_gdext.dylib\"\n"
)

# ---------------------------------------------------------------------------
# Copy GDExtension library to addons/bin/ (post-build)
# ---------------------------------------------------------------------------
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
set(CPACK_PACKAGE_FILE_NAME "addons")
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
set(CPACK_STRIP_FILES OFF)
include(CPack)

# ---------------------------------------------------------------------------
# Utility targets
# ---------------------------------------------------------------------------
add_custom_target(deep-clean
    COMMAND ${CMAKE_COMMAND} -E rm -rf "${_gdext_bin_dir}"
    COMMAND ${CMAKE_COMMAND} -E rm -rf "${CMAKE_CURRENT_BINARY_DIR}/_deps"
    COMMENT "Cleaning addons/bin/ and FetchContent cache"
)
