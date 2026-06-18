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
set(PROJECT_VERSION "0.2.1-dev4")
message(STATUS "GodotMCP version: ${PROJECT_VERSION}")

# ---------------------------------------------------------------------------
# Compiler launcher (sccache/ccache) — speeds up incremental builds 2-5x.
# Installed via: winget install sccache  |  choco install sccache
# ---------------------------------------------------------------------------
find_program(GODOTMCP_CXX_LAUNCHER NAMES sccache ccache)
if(GODOTMCP_CXX_LAUNCHER)
    message(STATUS "Compiler launcher: ${GODOTMCP_CXX_LAUNCHER} (cached builds)")
    set(CMAKE_C_COMPILER_LAUNCHER "${GODOTMCP_CXX_LAUNCHER}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${GODOTMCP_CXX_LAUNCHER}")
else()
    message(STATUS "Compiler launcher: not found (direct compilation)")
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
#   auto  (default) → Clang ThinLTO if available, else standard IPO
#   thin             → Clang ThinLTO (falls back to IPO on MSVC/GCC)
#   full             → Standard CMake IPO (full LTO)
# ---------------------------------------------------------------------------
set(GODOTMCP_LTO_MODE "auto" CACHE STRING
    "LTO mode: auto (detect best), thin (ThinLTO), full (Full LTO)")
set_property(CACHE GODOTMCP_LTO_MODE PROPERTY STRINGS auto thin full)

set(GODOTMCP_LTO "OFF" CACHE INTERNAL "")
include(CheckIPOSupported)

macro(_lto_ipo_fallback)
    check_ipo_supported(RESULT _ipo_ok OUTPUT _ipo_err)
    if(_ipo_ok)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
        set(GODOTMCP_LTO "ON (full)" CACHE INTERNAL "")
        message(STATUS "LTO: full (via IPO, Release only)")
    else()
        message(${ARGV0} "LTO not supported: ${_ipo_err}")
    endif()
endmacro()

if((GODOTMCP_LTO_MODE STREQUAL "auto" OR GODOTMCP_LTO_MODE STREQUAL "thin")
    AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options($<$<CONFIG:Release>:-flto=thin>)
    add_link_options($<$<CONFIG:Release>:-flto=thin>)
    set(GODOTMCP_LTO "ON (thin)" CACHE INTERNAL "")
    message(STATUS "LTO: thin (Clang ThinLTO, Release only)")
elseif(GODOTMCP_LTO_MODE STREQUAL "thin")
    _lto_ipo_fallback(WARNING "LTO mode 'thin' not available with this compiler")
else()
    _lto_ipo_fallback(STATUS)
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
