# ---------------------------------------------------------------------------
# GodotMCP GDExtension — C++ implementation using godot-cpp 10.0.0-rc1
# ---------------------------------------------------------------------------

include(FetchContent)

# Pin to the godot-cpp 10.0.0-rc1 release.
set(GODOTCPP_API_VERSION "4.6" CACHE STRING "Godot API version to target")

FetchContent_Declare(
    godot-cpp
    URL https://github.com/godotengine/godot-cpp/archive/refs/tags/10.0.0-rc1.zip
    URL_HASH SHA256=ddb8beb99b709e7c0c6831d62149bb225a9abec80e13621dc789cbcd9186e2a1
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(godot-cpp)

# ---------------------------------------------------------------------------
# rapidyaml (ryml) — YAML parser
# ---------------------------------------------------------------------------
set(RYML_HEADER_ONLY ON CACHE INTERNAL "Use ryml as header-only" FORCE)

FetchContent_Declare(
    rapidyaml
    GIT_REPOSITORY https://github.com/biojppm/rapidyaml.git
    GIT_TAG v0.7.0
    GIT_SHALLOW TRUE
    GIT_SUBMODULES "ext/c4core"
)
FetchContent_MakeAvailable(rapidyaml)
set_target_properties(ryml PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Patch ryml common.hpp for clang-cl on Windows
set(_ryml_common_hpp "${rapidyaml_SOURCE_DIR}/src/c4/yml/common.hpp")
if(EXISTS "${_ryml_common_hpp}")
    file(READ "${_ryml_common_hpp}" _ryml_common)
    string(REPLACE
        "#ifdef C4_MSVC\n#include <malloc.h>\n#else\n#include <alloca.h>\n#endif"
        "#if defined(C4_MSVC) || (defined(_WIN32) && defined(__clang__))\n#include <malloc.h>\n#else\n#include <alloca.h>\n#endif"
        _ryml_common "${_ryml_common}")
    file(WRITE "${_ryml_common_hpp}" "${_ryml_common}")
    message(STATUS "ryml: patched common.hpp for clang-cl on Windows")
endif()

# ---------------------------------------------------------------------------
# Source list
# ---------------------------------------------------------------------------
set(GODOT_MCP_SOURCES
    src/register_types.cpp
    src/editor_plugin.cpp
    src/sdk/mcp_tool_definition.cpp
    src/sdk/mcp_tool_registry.cpp
    src/server/ipc/http_server.cpp
    src/server/ipc/http_parser.cpp
    src/server/ipc/http_connection.cpp
    src/server/ipc/http_sse.cpp
    src/server/mcp/mcp_handler.cpp
    src/server/mcp/tool_executor.cpp
    src/server/mcp/prompt_provider.cpp
    src/server/registry/handler_registry.cpp
    src/built_in/tool_base.cpp
    src/built_in/tool_adapter.cpp
    src/built_in/register_itools.cpp
    src/built_in/cmd_utils.cpp
    src/built_in/cmd_utils_json.cpp
    src/runtime/bridge.cpp
    src/runtime/bridge_server.cpp
    src/runtime/game_bridge.cpp
    src/testing/test_engine.cpp
    src/pipeline/pipeline_parser.cpp
    src/pipeline/pipeline_context.cpp
    src/pipeline/workflow_parser.cpp
    src/pipeline/workflow_runner.cpp
    src/pipeline/test_parser.cpp
    src/pipeline/pipeline_runner_base.cpp
    src/testing/test_runner.cpp
    src/ui/mcp_logger.cpp
    src/ui/mcp_console.cpp
    src/ui/mcp_confirm_dialog.cpp
)

file(GLOB_RECURSE TOOL_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/built_in/tools/*.cpp")
list(APPEND GODOT_MCP_SOURCES ${TOOL_SOURCES})

add_library(godot_mcp_gdext SHARED ${GODOT_MCP_SOURCES})
set_source_files_properties(src/built_in/register_itools.cpp PROPERTIES SKIP_UNITY_BUILD ON)

target_link_libraries(godot_mcp_gdext PRIVATE godot-cpp ryml::ryml)

target_compile_definitions(godot_mcp_gdext PRIVATE
    GODOT_MCP_PLUGIN_VERSION="${PROJECT_VERSION}"
)

target_include_directories(godot_mcp_gdext PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${godot-cpp_SOURCE_DIR}/include
)

# Platform detection (arch, CI)
include(${CMAKE_SOURCE_DIR}/cmake/platform.cmake)

# Compiler-specific flags
include(${CMAKE_SOURCE_DIR}/cmake/compiler.cmake)

# Suppress warnings from godot-cpp generated headers (clang-cl)
target_compile_options(godot_mcp_gdext PRIVATE -Wno-unused-parameter -Wno-microsoft-template)

# Unity (jumbo) build
include(${CMAKE_SOURCE_DIR}/cmake/unity.cmake)

# Profile-Guided Optimization
include(${CMAKE_SOURCE_DIR}/cmake/pgo.cmake)

# C++ standard
set_target_properties(godot_mcp_gdext PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)
message(STATUS "C++ standard: 17")
message(STATUS "Generator: ${CMAKE_GENERATOR}")

# Linker optimization: lld-link on MSVC
if(MSVC AND NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    find_program(GODOTMCP_LLD_LINK NAMES lld-link)
    if(GODOTMCP_LLD_LINK)
        target_link_options(godot_mcp_gdext PRIVATE "/lldlink")
        message(STATUS "Linker: lld-link (fast)")
    else()
        message(STATUS "Linker: default (lld-link not found)")
    endif()
else()
    message(STATUS "Linker: lld-link (native, clang)")
endif()

# ---------------------------------------------------------------------------
# Build config for Python summary
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
    "compiler_launcher=${CMAKE_CXX_COMPILER_LAUNCHER}\n"
    "lto_mode=${GODOTMCP_LTO_MODE}\n"
    "lto=${GODOTMCP_LTO}\n"
    "unity_enabled=${_unity_enabled}\n"
    "unity_batch_count=${_unity_batch}\n"
    "unity_files=${_unity_files}\n"
    "lld_link=${GODOTMCP_LLD_LINK}\n"
    "ninja=${_ninja_found}\n"
)
