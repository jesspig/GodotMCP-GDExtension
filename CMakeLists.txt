cmake_minimum_required(VERSION 3.22)
project(GodotMCP LANGUAGES NONE)

# ---------------------------------------------------------------------------
# Options (equivalent to package_addons.py flags)
# ---------------------------------------------------------------------------
option(RELEASE    "Build with cargo --release profile" OFF)
option(NO_SERVER  "Skip building godot-mcp-server"     OFF)

# ---------------------------------------------------------------------------
# Version: single source of truth from Cargo.toml [workspace.package]
# ---------------------------------------------------------------------------
file(STRINGS "${CMAKE_SOURCE_DIR}/Cargo.toml" _cargo_lines)
set(_in_workspace_pkg FALSE)
foreach(_line ${_cargo_lines})
    if(_line STREQUAL "[workspace.package]")
        set(_in_workspace_pkg TRUE)
    elseif(_line MATCHES "^\\[")
        set(_in_workspace_pkg FALSE)
    elseif(_in_workspace_pkg AND _line MATCHES "^version *= *\"([^\"]+)\"")
        set(PROJECT_VERSION "${CMAKE_MATCH_1}")
        break()
    endif()
endforeach()
if(NOT PROJECT_VERSION)
    message(FATAL_ERROR "Cannot parse version from Cargo.toml [workspace.package]")
endif()
message(STATUS "Project version (from Cargo.toml): ${PROJECT_VERSION}")

# ---------------------------------------------------------------------------
# Corrosion: bridge between CMake and Cargo
# ---------------------------------------------------------------------------
include(FetchContent)
FetchContent_Declare(
    Corrosion
    GIT_REPOSITORY https://github.com/corrosion-rs/corrosion.git
    GIT_TAG        v0.6
)
FetchContent_MakeAvailable(Corrosion)

# ---------------------------------------------------------------------------
# Cargo profile selection
# ---------------------------------------------------------------------------
if(RELEASE)
    set(_cargo_profile "release")
else()
    set(_cargo_profile "dev")
endif()

# ---------------------------------------------------------------------------
# Kill running server process so cargo can overwrite the binary
# ---------------------------------------------------------------------------
if(NOT NO_SERVER)
    if(WIN32)
        execute_process(
            COMMAND taskkill /F /IM godot-mcp-server.exe /T
            ERROR_QUIET OUTPUT_QUIET
        )
    else()
        execute_process(
            COMMAND pkill -f godot-mcp-server
            ERROR_QUIET OUTPUT_QUIET
        )
    endif()
endif()

# ---------------------------------------------------------------------------
# Import Rust crates as CMake targets
# ---------------------------------------------------------------------------
# Always build the GDExtension (cdylib)
corrosion_import_crate(
    MANIFEST_PATH "${CMAKE_SOURCE_DIR}/Cargo.toml"
    CRATES        godot-mcp-gdext
    PROFILE       ${_cargo_profile}
)

# Optionally build the MCP server (binary)
if(NOT NO_SERVER)
    corrosion_import_crate(
        MANIFEST_PATH "${CMAKE_SOURCE_DIR}/Cargo.toml"
        CRATES        godot-mcp-server
        PROFILE       ${_cargo_profile}
    )
endif()

# ---------------------------------------------------------------------------
# Generate addon config files (at configure time)
# ---------------------------------------------------------------------------
set(_addon_dir "${CMAKE_SOURCE_DIR}/example/addons/godot_mcp")
file(MAKE_DIRECTORY "${_addon_dir}")

# plugin.cfg
file(WRITE "${_addon_dir}/plugin.cfg"
    "[plugin]\n"
    "name=\"Godot MCP\"\n"
    "description=\"Model Context Protocol bridge for Godot Engine.\"\n"
    "author=\"\"\n"
    "version=\"${PROJECT_VERSION}\"\n"
    "script=\"\"\n"
)

# godot_mcp.gdextension
file(WRITE "${_addon_dir}/godot_mcp.gdextension"
    "[configuration]\n"
    "\n"
    "entry_symbol = \"gdext_rust_init\"\n"
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
set(_gdext_bin_dir "${_addon_dir}/bin")
file(MAKE_DIRECTORY "${_gdext_bin_dir}")

# Corrosion copies artifacts to ${CMAKE_CURRENT_BINARY_DIR}. The INTERFACE target
# created for cdylib does not support POST_BUILD, so we use a separate custom target
# that depends on the Corrosion-internal cargo build target.
if(WIN32)
    set(_gdext_lib_name "godot_mcp_gdext.dll")
elseif(APPLE)
    set(_gdext_lib_name "libgodot_mcp_gdext.dylib")
else()
    set(_gdext_lib_name "libgodot_mcp_gdext.so")
endif()

add_custom_target(copy-gdext ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_BINARY_DIR}/${_gdext_lib_name}"
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
)

# ---------------------------------------------------------------------------
# CPack configuration -> addons.zip
# ---------------------------------------------------------------------------
set(CPACK_GENERATOR "ZIP")
set(CPACK_PACKAGE_FILE_NAME "addons")
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
set(CPACK_STRIP_FILES OFF)
include(CPack)

# ---------------------------------------------------------------------------
# Utility targets
# ---------------------------------------------------------------------------
add_custom_target(deep-clean
    COMMAND ${CMAKE_COMMAND} -E echo "Running cargo clean..."
    COMMAND cargo clean
    COMMAND ${CMAKE_COMMAND} -E rm -rf "${_gdext_bin_dir}"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    COMMENT "Cleaning cargo build artifacts and addons/bin/"
)
