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
    GIT_SUBMODULES "ext/c4core"
)

# Ensure rapidyaml/c4core static libs are compiled with -fPIC (needed for shared lib linking on Linux/macOS).
FetchContent_MakeAvailable(rapidyaml)

set_target_properties(ryml PROPERTIES POSITION_INDEPENDENT_CODE ON)

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
    src/server/mcp/tool_executor.cpp
    src/server/mcp/prompt_provider.cpp
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

set_source_files_properties(src/built_in/register_itools.cpp PROPERTIES SKIP_UNITY_BUILD ON)

target_link_libraries(godot_mcp_gdext PRIVATE godot-cpp ryml::ryml)

target_include_directories(godot_mcp_gdext PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${godot-cpp_SOURCE_DIR}/include
)





# ---------------------------------------------------------------------------
# Build optimization options (user-configurable via -D or env)
# ---------------------------------------------------------------------------
option(GODOTMCP_UNITY_BUILD "Enable unity (jumbo) build — faster full rebuilds" ON)
set(GODOTMCP_UNITY_BATCH_SIZE "0" CACHE STRING
    "Unity batch count (0 = adaptive: min(cpu, ceil(files/4)))")

# Adaptive unity batch sizing:
#   1 core        → unity disabled (single-thread, no batching benefit)
#   2+ cores      → batch_count = min(CPU cores, ceil(unity_files / 4))
#   Each batch gets ≥4 files for effective header amortization.
#   File count naturally bounds batches on any machine — no hard cap needed.
set(_unity_batch ${GODOTMCP_UNITY_BATCH_SIZE})
set(_unity_enabled ${GODOTMCP_UNITY_BUILD})
set(_unity_files 0)
if(_unity_batch STREQUAL "0")
    include(ProcessorCount)
    ProcessorCount(_cpu_count)

    # Count files participating in unity build (SKIP_UNITY_BUILD excluded)
    list(LENGTH GODOT_MCP_SOURCES _src_count)
    math(EXPR _unity_files "${_src_count} - 1")

    if(_cpu_count LESS 2)
        # Single core: unity build offers no benefit, disable entirely
        set(_unity_batch 1)
        set(_unity_enabled OFF)
        message(STATUS "Unity build disabled (1 core)")
    else()
        # ceil(unity_files / 4) — each batch gets ≥4 files
        math(EXPR _max_batches "(${_unity_files} + 3) / 4")
        if(_max_batches LESS 2)
            set(_max_batches 2)
        endif()

        if(_cpu_count GREATER _max_batches)
            set(_cpu_count ${_max_batches})
        endif()

        set(_unity_batch ${_cpu_count})
        message(STATUS "Unity batch size: ${_unity_batch} (${_unity_files} files, max ${_max_batches} batches)")
    endif()
else()
    message(STATUS "Unity batch size: ${_unity_batch} (user-specified)")
endif()

set_target_properties(godot_mcp_gdext PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    UNITY_BUILD ${_unity_enabled}
    UNITY_BUILD_BATCH_SIZE ${_unity_batch}
)
message(STATUS "C++ standard: 17")
message(STATUS "Generator: ${CMAKE_GENERATOR}")

# ---------------------------------------------------------------------------
# Linker optimization: prefer lld-link on MSVC for faster linking.
# lld-link ships with LLVM (winget install LLVM).
# ---------------------------------------------------------------------------
if(MSVC)
    find_program(GODOTMCP_LLD_LINK NAMES lld-link)
    if(GODOTMCP_LLD_LINK)
        target_link_options(godot_mcp_gdext PRIVATE "/lldlink")
        message(STATUS "Linker: lld-link (fast)")
    else()
        message(STATUS "Linker: default (lld-link not found, install LLVM)")
    endif()
endif()

if(MSVC)
    target_compile_options(godot_mcp_gdext PRIVATE /utf-8 /bigobj /W4 /wd4244 /wd4267)
else()
    target_compile_options(godot_mcp_gdext PRIVATE -Wall -Wextra -Wno-unused-parameter)
endif()

# ---------------------------------------------------------------------------
# PGO (Profile-Guided Optimization) — only for Release + GODOTMCP_PGO
# Usage:
#   1. cmake -B build -DCMAKE_BUILD_TYPE=Release -DGODOTMCP_PGO=GEN
#   2. cmake --build build
#   3. Load godot_mcp_gdext.dll into Godot, exercise workflows
#   4. cmake -B build -DCMAKE_BUILD_TYPE=Release -DGODOTMCP_PGO=USE
#   5. cmake --build build
# ---------------------------------------------------------------------------
if(MSVC AND CMAKE_BUILD_TYPE STREQUAL "Release" AND GODOTMCP_PGO STREQUAL "GEN")
    target_compile_options(godot_mcp_gdext PRIVATE /GL)
    target_link_options(godot_mcp_gdext PRIVATE /LTCG /GENPROFILE)
    message(STATUS "PGO: instrumentation build (GEN)")
elseif(MSVC AND CMAKE_BUILD_TYPE STREQUAL "Release" AND GODOTMCP_PGO STREQUAL "USE")
    target_compile_options(godot_mcp_gdext PRIVATE /GL)
    target_link_options(godot_mcp_gdext PRIVATE /LTCG /USEPROFILE)
    message(STATUS "PGO: optimization build (USE)")
endif()

# ---------------------------------------------------------------------------
# Write build config for Python summary (read by main.py after configure)
# ---------------------------------------------------------------------------
if(CMAKE_GENERATOR MATCHES "Ninja")
    set(_ninja_found "ON")
else()
    set(_ninja_found "OFF")
endif()
file(WRITE "${CMAKE_BINARY_DIR}/build_config.txt"
    "version=${PROJECT_VERSION}\n"
    "build_type=${CMAKE_BUILD_TYPE}\n"
    "generator=${CMAKE_GENERATOR}\n"
    "compiler_id=${CMAKE_CXX_COMPILER_ID}\n"
    "cxx_standard=17\n"
    "compiler_launcher=${GODOTMCP_CXX_LAUNCHER}\n"
    "lto_mode=${GODOTMCP_LTO_MODE}\n"
    "lto=${GODOTMCP_LTO}\n"
    "unity_enabled=${_unity_enabled}\n"
    "unity_batch_count=${_unity_batch}\n"
    "unity_files=${_unity_files}\n"
    "lld_link=${GODOTMCP_LLD_LINK}\n"
    "ninja=${_ninja_found}\n"
)

# Unity build: third-party headers are included in our unity batch,
# so warnings from godot-cpp/ryml headers cannot be suppressed per-target.
# /wd4244 is noisy in godot-cpp (template metaprogramming), but we want it
# for our own code. /wd4267 similarly.
# TODO: Gradually fix C4244/C4267 in our own code, then re-enable for our target.
