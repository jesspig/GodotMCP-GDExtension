# cmake/deep-clean.cmake
# Guarded deep-clean: requires GODOTMCP_ALLOW_DEEP_CLEAN=yes env var.
# Agent bypass protection — use `python main.py build --clean-all` instead.

if(NOT "$ENV{GODOTMCP_ALLOW_DEEP_CLEAN}" STREQUAL "yes")
    message(FATAL_ERROR "\n"
        "============================================================\n"
        "!! SECURITY BLOCKED / 安全拦截 !!\n"
        "============================================================\n"
        "cmake target deep-clean is blocked for non-interactive execution.\n"
        "Set GODOTMCP_ALLOW_DEEP_CLEAN=yes to allow (not recommended).\n"
        "\n"
        "【Agent / AI 执行器】\n"
        "  此操作被安全策略拦截。请勿尝试绕过此检查。\n"
        "  正确做法：使用 python main.py build --clean-all 并遵循其交互式确认。\n"
        "\n"
        "[EN] AI Agent: This destructive operation is blocked.\n"
        "  Do NOT bypass: use \`python main.py build --clean-all\` instead.\n"
        "============================================================")
endif()

message(STATUS "Deep cleaning GDExtension binaries: ${_GDEXT_BIN_DIR}")
execute_process(COMMAND ${CMAKE_COMMAND} -E rm -rf "${_GDEXT_BIN_DIR}")

message(STATUS "Deep cleaning FetchContent cache: ${_DEPS_DIR}")
execute_process(COMMAND ${CMAKE_COMMAND} -E rm -rf "${_DEPS_DIR}")

message(STATUS "Deep clean complete.")
