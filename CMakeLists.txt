cmake_minimum_required(VERSION 3.22)
project(GodotMCP LANGUAGES CXX C)

# ---------------------------------------------------------------------------
# Options
# ---------------------------------------------------------------------------
option(RELEASE    "Build with Release configuration" OFF)
option(NO_SERVER  "Skip building godot-mcp-server"   OFF)

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
set(PROJECT_VERSION "0.1.5-dev2")
message(STATUS "GodotMCP version: ${PROJECT_VERSION}")

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
# Force .venv Python for ALL sub-projects (godot-cpp's internal find_package
# would otherwise pick up the system Python and cache it first).
# ---------------------------------------------------------------------------
set(_venv_python "${CMAKE_SOURCE_DIR}/.venv/Scripts/python.exe")
if(EXISTS "${_venv_python}")
    set(Python3_EXECUTABLE "${_venv_python}" CACHE PATH "Python3 from .venv" FORCE)
    message(STATUS "Using .venv Python: ${Python3_EXECUTABLE}")
else()
    message(STATUS ".venv Python not found, relying on system PATH")
endif()

# ---------------------------------------------------------------------------
# C++ GDExtension (godot-cpp 10.0.0-rc1 via FetchContent)
# ---------------------------------------------------------------------------
add_subdirectory(extensions/gdext)

# ---------------------------------------------------------------------------
# Build Python MCP server via Cython --embed
# ---------------------------------------------------------------------------
set(_addon_dir "${CMAKE_SOURCE_DIR}/example/addons/godot_mcp")
set(_gdext_bin_dir "${_addon_dir}/bin")
file(MAKE_DIRECTORY "${_addon_dir}")
file(MAKE_DIRECTORY "${_gdext_bin_dir}")

if(NOT NO_SERVER)
    find_package(Python3 REQUIRED COMPONENTS Interpreter Development)

    set(_server_src   "${CMAKE_SOURCE_DIR}/server")
    set(_server_entry "${_server_src}/entry.py")
    set(_server_c     "${CMAKE_CURRENT_BINARY_DIR}/entry.c")
    set(_server_exe   "${CMAKE_CURRENT_BINARY_DIR}/godot-mcp-server.exe")

    set(_patch_script "${_server_src}/tools/patch_entry_c.py")
    set(_server_c_patched "${CMAKE_CURRENT_BINARY_DIR}/entry_patched.c")
    # Python DLL lives alongside python3*.exe in sys.base_prefix, NOT in venv\Scripts.
    execute_process(
        COMMAND "${Python3_EXECUTABLE}" -c "import sys; print(sys.base_prefix)"
        OUTPUT_VARIABLE _python3_base_prefix
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    # Step 1: Cython --embed -> entry.c
    add_custom_command(
        OUTPUT  ${_server_c}
        COMMAND "${_venv_python}" -m cython
            --embed -o ${_server_c}
            ${_server_entry}
        DEPENDS ${_server_entry}
        COMMENT "Generating entry.c via Cython --embed"
    )

    # Step 2: Patch PYTHONHOME into entry.c
    add_custom_command(
        OUTPUT  ${_server_c_patched}
        COMMAND "${Python3_EXECUTABLE}"
            ${_patch_script}
            ${_server_c}
            ${_server_c_patched}
            "${_python3_base_prefix}"
        DEPENDS ${_server_c} ${_patch_script}
        COMMENT "Patching entry.c with Python home"
    )

    # Step 3: Compile patched C -> exe
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

    add_custom_target(build-python-server ALL DEPENDS ${_server_exe})

    # Step 4: Copy python3 DLL alongside the exe
    # Python DLL naming: python{major}{minor}.dll (e.g., python314.dll for 3.14.x)
    if(WIN32)
        string(REGEX MATCH "^([0-9]+)\\.([0-9]+)" _python_ver_match "${Python3_VERSION}")
        set(_python_dll_name "python${CMAKE_MATCH_1}${CMAKE_MATCH_2}.dll")
        set(_python_dll_src "${_python3_base_prefix}/${_python_dll_name}")
        add_custom_command(
            TARGET build-python-server POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${_python_dll_src}"
                "${CMAKE_CURRENT_BINARY_DIR}/${_python_dll_name}"
            COMMENT "Copying ${_python_dll_name} alongside server exe"
        )
    endif()
endif()

# ---------------------------------------------------------------------------
# Generate addon config files (plugin.cfg, .gdextension)
# ---------------------------------------------------------------------------
file(WRITE "${_addon_dir}/plugin.cfg"
    "[plugin]\n"
    "name=\"Godot MCP\"\n"
    "description=\"Model Context Protocol bridge for Godot Engine.\"\n"
    "author=\"\"\n"
    "version=\"${PROJECT_VERSION}\"\n"
    "script=\"\"\n"
)

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
