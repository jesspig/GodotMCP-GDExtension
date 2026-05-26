cmake_minimum_required(VERSION 3.22)
project(GodotMCP LANGUAGES C)

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
# Kill running server process so the new binary can overwrite
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
# GDExtension (cdylib) — always built via Cargo
corrosion_import_crate(
    MANIFEST_PATH "${CMAKE_SOURCE_DIR}/Cargo.toml"
    CRATES        godot-mcp-gdext
    PROFILE       ${_cargo_profile}
)

# ---------------------------------------------------------------------------
# Build Python MCP server via Cython --embed (replaces old Rust server)
# ---------------------------------------------------------------------------
set(_addon_dir "${CMAKE_SOURCE_DIR}/example/addons/godot_mcp")
set(_gdext_bin_dir "${_addon_dir}/bin")
if(NOT NO_SERVER)
    find_package(Python3 REQUIRED COMPONENTS Interpreter Development)

    set(_server_src   "${CMAKE_SOURCE_DIR}/server")
    set(_server_entry "${_server_src}/entry.py")
    set(_server_c     "${CMAKE_CURRENT_BINARY_DIR}/entry.c")
    set(_server_exe   "${CMAKE_CURRENT_BINARY_DIR}/godot-mcp-server.exe")

    # Step 1: Cython --embed → entry.c
    set(_patch_script "${_server_src}/tools/patch_entry_c.py")
    set(_server_c_patched "${CMAKE_CURRENT_BINARY_DIR}/entry_patched.c")
    get_filename_component(_python3_home "${Python3_EXECUTABLE}" DIRECTORY)
    # Use .venv Python (has Cython installed, now Python 3.14 matching system)
    set(_venv_python "${_server_src}/.venv/Scripts/python.exe")
    if(NOT EXISTS "${_venv_python}")
        set(_venv_python "${Python3_EXECUTABLE}")
    endif()
    add_custom_command(
        OUTPUT  ${_server_c}
        COMMAND "${_venv_python}" -m cython
            --embed -o ${_server_c}
            ${_server_entry}
        DEPENDS ${_server_entry}
        COMMENT "Generating entry.c via Cython --embed (${_server_entry})"
    )

    # Step 2: Patch entry.c to embed PYTHONHOME
    add_custom_command(
        OUTPUT  ${_server_c_patched}
        COMMAND "${Python3_EXECUTABLE}"
            ${_patch_script}
            ${_server_c}
            ${_server_c_patched}
            "${_python3_home}"
        DEPENDS ${_server_c} ${_patch_script}
        COMMENT "Patching entry.c with Python home: ${_python3_home}"
    )

    # Step 3: Compile patched C file → .exe
    add_custom_command(
        OUTPUT  ${_server_exe}
        COMMAND ${CMAKE_C_COMPILER}
            -I"${Python3_INCLUDE_DIRS}"
            ${_server_c_patched}
            -o ${_server_exe}
            "${Python3_LIBRARIES}"
        DEPENDS ${_server_c_patched}
        COMMENT "Compiling godot-mcp-server.exe"
    )

    add_custom_target(build-python-server ALL
        DEPENDS ${_server_exe}
    )

    # Step 4: Copy python3 DLL alongside .exe so it can find it at runtime
    add_custom_command(
        TARGET build-python-server POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${_python3_home}/python314.dll"
            "${CMAKE_CURRENT_BINARY_DIR}/python314.dll"
        COMMENT "Copying python314.dll alongside server exe"
    )
endif()

# ---------------------------------------------------------------------------
# Generate addon config files (at configure time)
# ---------------------------------------------------------------------------
file(MAKE_DIRECTORY "${_addon_dir}")
file(MAKE_DIRECTORY "${_gdext_bin_dir}")

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
