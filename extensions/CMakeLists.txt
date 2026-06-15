# ---------------------------------------------------------------------------
# GodotMCP GDExtension — C++ implementation using godot-cpp 10.0.0-rc1
# ---------------------------------------------------------------------------
# Targets Godot 4.6+ via the godot-cpp v10 bindings (FetchContent).
# Produces a single shared library (godot_mcp_gdext.dll / .so / .dylib) that
# is copied to addons/godot_mcp/bin/ by the parent CMakeLists.txt.
# ---------------------------------------------------------------------------

include(FetchContent)

# Pin to the godot-cpp 10.0.0-rc1 release.
# v10 follows independent versioning; this release is synced with Godot 4.6-stable
# and supports api_version 4.3 / 4.4 / 4.5 / 4.6.
# Target Godot 4.6 API. Must be set before FetchContent_MakeAvailable.
set(GODOTCPP_API_VERSION "4.6" CACHE STRING "Godot API version to target")

FetchContent_Declare(
    godot-cpp
    URL https://github.com/godotengine/godot-cpp/archive/refs/tags/10.0.0-rc1.zip
    URL_HASH SHA256=ddb8beb99b709e7c0c6831d62149bb225a9abec80e13621dc789cbcd9186e2a1
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

FetchContent_MakeAvailable(godot-cpp)

# ---------------------------------------------------------------------------
# rapidyaml (ryml) — YAML parser for the test engine
# ---------------------------------------------------------------------------

# Header-only to avoid compiling c4core sources.
set(RYML_HEADER_ONLY ON CACHE INTERNAL "Use ryml as header-only" FORCE)

# Must use GIT (not URL zip) so the ext/c4core submodule is fetched.
FetchContent_Declare(
    rapidyaml
    GIT_REPOSITORY https://github.com/biojppm/rapidyaml.git
    GIT_TAG v0.7.0
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
    GIT_SUBMODULES "ext/c4core"
)

# Ensure rapidyaml/c4core static libs are compiled with -fPIC (needed for shared lib linking on Linux/macOS).
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
FetchContent_MakeAvailable(rapidyaml)

# ---------------------------------------------------------------------------
# Source list — all .cpp files for the GDExtension.
# Add new translation units here as the implementation grows.
# ---------------------------------------------------------------------------
set(GODOT_MCP_SOURCES
    # Entry point
    src/register_types.cpp
    src/editor_plugin.cpp

    # SDK
    src/sdk/mcp_tool_definition.cpp
    src/sdk/mcp_tool_registry.cpp

    # Server
    src/server/ipc/http_server.cpp
    src/server/ipc/http_parser.cpp
    src/server/ipc/http_connection.cpp
    src/server/ipc/http_sse.cpp
    src/server/mcp/mcp_handler.cpp
    src/server/registry/handler_registry.cpp

    # Tool infrastructure (ITool)
    src/built_in/tool_base.cpp
    src/built_in/tool_adapter.cpp
    src/built_in/register_itools.cpp

    # Utility functions
    src/built_in/cmd_utils.cpp
    src/built_in/cmd_utils_json.cpp

    # Runtime bridge
    src/runtime/bridge.cpp
    src/runtime/game_bridge.cpp

    # Testing engine
    src/testing/test_engine.cpp

    # UI
    src/ui/mcp_dock.cpp
    src/ui/mcp_logger.cpp
    src/ui/mcp_console.cpp
)

# ---------------------------------------------------------------------------
# GLOB for ITool-based tool source files (P2+ migration)
# ---------------------------------------------------------------------------
file(GLOB_RECURSE TOOL_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/built_in/tools/*.cpp")
list(APPEND GODOT_MCP_SOURCES ${TOOL_SOURCES})



add_library(godot_mcp_gdext SHARED ${GODOT_MCP_SOURCES})

target_link_libraries(godot_mcp_gdext PRIVATE godot-cpp ryml::ryml)

target_include_directories(godot_mcp_gdext PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${godot-cpp_SOURCE_DIR}/include
)

# Plugin version baked in for runtime reporting via get_plugin_version tool.
# Falls back to PROJECT_VERSION set in the top-level CMakeLists.txt.
if(NOT DEFINED GODOT_MCP_PLUGIN_VERSION)
    set(GODOT_MCP_PLUGIN_VERSION "${PROJECT_VERSION}")
endif()
target_compile_definitions(godot_mcp_gdext PRIVATE
    GODOT_MCP_PLUGIN_VERSION="${GODOT_MCP_PLUGIN_VERSION}"
)

# Suppress CRT deprecation warnings (fopen, strcpy, etc.) on MSVC.
# Only needed if raw CRT calls are present; remove if codebase uses Godot API exclusively.
if(MSVC)
    target_compile_definitions(godot_mcp_gdext PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# ---------------------------------------------------------------------------
# Build optimization options (user-configurable via -D or env)
# ---------------------------------------------------------------------------
option(GODOTMCP_UNITY_BUILD "Enable unity (jumbo) build — faster full rebuilds" ON)
set(GODOTMCP_UNITY_BATCH_SIZE "0" CACHE STRING
    "Unity batch size (source files per batch; 0 = all in one, max parallelism)")

# Auto-tune unity batch size: batch_size=0 means "match CPU count".
# With N CPU cores and N batches, every core gets exactly one job.
# This maximizes parallelism with minimal memory pressure.
set(_unity_batch ${GODOTMCP_UNITY_BATCH_SIZE})
if(_unity_batch STREQUAL "0")
    include(ProcessorCount)
    ProcessorCount(_cpu_count)
    # Cap at 32 — beyond that, batches get too large (memory / ODR risk).
    if(_cpu_count GREATER 32)
        set(_cpu_count 32)
    endif()
    # Ensure at least 2 batches for incremental build efficiency
    if(_cpu_count LESS 2)
        set(_cpu_count 2)
    endif()
    set(_unity_batch ${_cpu_count})
    message(STATUS "Unity batch size: ${_unity_batch} (auto = ${_cpu_count} cores)")
else()
    message(STATUS "Unity batch size: ${_unity_batch} (user-specified)")
endif()

set_target_properties(godot_mcp_gdext PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    UNITY_BUILD ${GODOTMCP_UNITY_BUILD}
    UNITY_BUILD_BATCH_SIZE ${_unity_batch}
    PREFIX ""
)
# Restore the conventional "lib" prefix on Unix.
if(UNIX)
    set_target_properties(godot_mcp_gdext PROPERTIES PREFIX "lib")
endif()

# ---------------------------------------------------------------------------
# Linker optimization: prefer lld-link on MSVC for faster linking.
# lld-link ships with LLVM (winget install LLVM).
# ---------------------------------------------------------------------------
if(MSVC)
    find_program(GODOTMCP_LLD_LINK NAMES lld-link)
    if(GODOTMCP_LLD_LINK)
        target_link_options(godot_mcp_gdext PRIVATE "/lldlink")
        message(STATUS "Using lld-link for faster linking")
    endif()
endif()

if(MSVC)
    target_compile_options(godot_mcp_gdext PRIVATE /W3 /wd4244 /wd4267)
else()
    target_compile_options(godot_mcp_gdext PRIVATE -Wall -Wno-unused-parameter)
endif()
