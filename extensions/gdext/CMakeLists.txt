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
# Source list — all .cpp files for the GDExtension.
# Add new translation units here as the implementation grows.
# ---------------------------------------------------------------------------
set(GODOT_MCP_SOURCES
    src/register_types.cpp
    src/editor_plugin.cpp
    src/ipc/http_server.cpp
    src/mcp/mcp_handler.cpp
    src/commands/handler_registry.cpp
    src/commands/cmd_utils.cpp
    src/commands/cmd_utils_json.cpp
    src/commands/meta.cpp
    src/commands/property.cpp
    src/commands/property_3d.cpp
    src/commands/undo.cpp
    src/commands/collision.cpp
    src/commands/find.cpp
    src/commands/script_helpers.cpp
    src/commands/editor_control.cpp
    src/commands/plugin_management.cpp
    src/commands/project_settings.cpp
    src/commands/project_settings_ext.cpp
    src/commands/script_gd.cpp
    src/commands/input_map.cpp
    src/commands/search.cpp
    src/commands/node.cpp
    src/commands/scene.cpp
    src/commands/script_cs.cpp
    src/lsp/client.cpp
)

add_library(godot_mcp_gdext SHARED ${GODOT_MCP_SOURCES})

target_link_libraries(godot_mcp_gdext PRIVATE godot-cpp)

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
    target_compile_options(godot_mcp_gdext PRIVATE /W3 /wd4244 /wd4267)
else()
    target_compile_options(godot_mcp_gdext PRIVATE -Wall -Wno-unused-parameter)
endif()
