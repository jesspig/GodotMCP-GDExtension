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

    # Legacy utilities (still needed by ITool implementations)
    src/built_in/cmd_utils.cpp
    src/built_in/cmd_utils_json.cpp

    # LSP
    src/lsp/client.cpp

    # Testing engine
    src/testing/test_engine.cpp

    # Plugin: TestRunnerDock
    src/plugin/test_runner_dock.cpp
)

# ---------------------------------------------------------------------------
# GLOB for ITool-based tool source files (P2+ migration)
# ---------------------------------------------------------------------------
file(GLOB_RECURSE TOOL_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/built_in/tools/*.cpp")
list(APPEND GODOT_MCP_SOURCES ${TOOL_SOURCES})

# ---------------------------------------------------------------------------
# Code generation: auto-registration from // @tool register comments + YAML db
# ---------------------------------------------------------------------------
find_package(Python3 COMPONENTS Interpreter QUIET)

set(CODEGEN_SCRIPT "${CMAKE_SOURCE_DIR}/tools/codegen.py")
set(CODEGEN_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/generated/generated_registration.cpp")
set(NODE_PROPS_DB "${CMAKE_CURRENT_SOURCE_DIR}/src/built_in/tools/node_props/db")
set(RESOURCE_PROPS_DB "${CMAKE_CURRENT_SOURCE_DIR}/src/built_in/tools/node_resource/db")

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated")

if(Python3_FOUND)
    file(GLOB_RECURSE TOOL_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/src/built_in/tools/*.hpp")
    file(GLOB_RECURSE NODE_PROP_YAMLS "${NODE_PROPS_DB}/*.yaml" "${NODE_PROPS_DB}/*.yml")
    file(GLOB_RECURSE RESOURCE_PROP_YAMLS "${RESOURCE_PROPS_DB}/*.yaml" "${RESOURCE_PROPS_DB}/*.yml")
    add_custom_command(
        OUTPUT ${CODEGEN_OUTPUT}
        COMMAND ${Python3_EXECUTABLE} ${CODEGEN_SCRIPT}
            --source-dir "${CMAKE_CURRENT_SOURCE_DIR}/src/built_in/tools"
            --node-props-db "${NODE_PROPS_DB}"
            --resource-props-db "${RESOURCE_PROPS_DB}"
            --output "${CODEGEN_OUTPUT}"
        DEPENDS ${CODEGEN_SCRIPT} ${TOOL_HEADERS} ${NODE_PROP_YAMLS} ${RESOURCE_PROP_YAMLS}
        COMMENT "Generating ITool registration code from // @tool register + YAML db..."
    )
    list(APPEND GODOT_MCP_SOURCES ${CODEGEN_OUTPUT})
    set_source_files_properties(${CODEGEN_OUTPUT} PROPERTIES GENERATED TRUE)
endif()

add_library(godot_mcp_gdext SHARED ${GODOT_MCP_SOURCES})

target_link_libraries(godot_mcp_gdext PRIVATE godot-cpp ryml::ryml)

target_include_directories(godot_mcp_gdext PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}/generated
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

set_target_properties(godot_mcp_gdext PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    POSITION_INDEPENDENT_CODE ON
    # Drop the "lib" prefix on Unix to match the .gdextension declarations
    # (Godot expects libgodot_mcp_gdext.so → keep prefix; on Windows there is none anyway).
    UNITY_BUILD ON
    UNITY_BUILD_BATCH_SIZE 8
    PREFIX ""
)
# Restore the conventional "lib" prefix on Unix.
if(UNIX)
    set_target_properties(godot_mcp_gdext PROPERTIES PREFIX "lib")
endif()

# Suppress noisy warnings from godot-cpp generated code on MSVC.
if(MSVC)
    target_compile_options(godot_mcp_gdext PRIVATE /utf-8 /bigobj /W3 /wd4244 /wd4267)
else()
    target_compile_options(godot_mcp_gdext PRIVATE -Wall -Wno-unused-parameter)
endif()
