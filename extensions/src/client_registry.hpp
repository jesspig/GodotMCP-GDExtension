#pragma once

#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot_mcp {

using ConfigGenerator = String (*)(const String &url);

struct ClientEntry {
    const char *name;
    const char *display_name;
    const char *config_path;
    const char *format;
    ConfigGenerator generator;
};

// ---- Generator implementations -------------------------------------------------

inline String generate_claude_code_config(const String &url) {
    Dictionary servers;
    Dictionary godot;
    godot["type"] = "http";
    godot["url"] = url;
    servers["godot"] = godot;
    Dictionary root;
    root["mcpServers"] = servers;
    return JSON::stringify(root, "  ");
}

inline String generate_cursor_config(const String &url) {
    Dictionary servers;
    Dictionary godot;
    godot["url"] = url;
    servers["godot"] = godot;
    Dictionary root;
    root["mcpServers"] = servers;
    return JSON::stringify(root, "  ");
}

inline String generate_vscode_config(const String &url) {
    Dictionary servers;
    Dictionary godot;
    godot["type"] = "http";
    godot["url"] = url;
    servers["godot"] = godot;
    Dictionary root;
    root["servers"] = servers;
    return JSON::stringify(root, "  ");
}

inline String generate_cline_config(const String &url) {
    Dictionary servers;
    Dictionary godot;
    godot["command"] = "npx";
    Array args;
    args.push_back("-y");
    args.push_back("mcp-remote");
    args.push_back(url);
    godot["args"] = args;
    servers["godot"] = godot;
    Dictionary root;
    root["mcpServers"] = servers;
    return JSON::stringify(root, "  ");
}

inline String generate_opencode_config(const String &url) {
    Dictionary servers;
    Dictionary godot;
    godot["type"] = "remote";
    godot["url"] = url;
    servers["godot"] = godot;
    Dictionary root;
    root["mcp"] = servers;
    return JSON::stringify(root, "  ");
}

inline String generate_codex_toml(const String &url) {
    String toml;
    toml += "[mcp_servers.godot]\n";
    toml += "enabled = true\n";
    toml += String("url = \"") + url + String("\"\n");
    toml += "transport = \"streamable_http\"\n";
    return toml;
}

inline String generate_trae_config(const String &url) {
    Dictionary servers;
    Dictionary godot;
    godot["url"] = url;
    servers["godot"] = godot;
    Dictionary root;
    root["mcpServers"] = servers;
    return JSON::stringify(root, "  ");
}

inline String generate_qoder_config(const String &url) {
    Dictionary servers;
    Dictionary godot;
    godot["transport"] = "http";
    godot["url"] = url;
    servers["godot"] = godot;
    Dictionary root;
    root["mcpServers"] = servers;
    return JSON::stringify(root, "  ");
}

inline String generate_codebuddy_config(const String &url) {
    Dictionary servers;
    Dictionary godot;
    godot["type"] = "stdio";
    godot["command"] = "npx";
    Array args;
    args.push_back("-y");
    args.push_back("mcp-remote");
    args.push_back(url);
    godot["args"] = args;
    servers["godot"] = godot;
    Dictionary root;
    root["mcpServers"] = servers;
    return JSON::stringify(root, "  ");
}

inline String generate_pi_config(const String &url) {
    Dictionary servers;
    Dictionary godot;
    godot["url"] = url;
    servers["godot"] = godot;
    Dictionary mcp;
    mcp["mcpServers"] = servers;
    Dictionary root;
    root["mcp"] = mcp;
    return JSON::stringify(root, "  ");
}

inline String generate_openclaw_config(const String &url) {
    Dictionary servers;
    Dictionary godot;
    godot["url"] = url;
    servers["godot"] = godot;
    Dictionary root;
    root["mcpServers"] = servers;
    return JSON::stringify(root, "  ");
}

// ---- Registry -------------------------------------------------------------------

inline const ClientEntry* get_entries(int &count) {
    static const ClientEntry entries[] = {
        {"claude_code",    "Claude Code",     ".mcp.json",                      "json", &generate_claude_code_config},
        {"cursor",         "Cursor",          ".cursor/mcp.json",               "json", &generate_cursor_config},
        {"vscode_copilot", "VS Code Copilot", ".vscode/mcp.json",               "json", &generate_vscode_config},
        {"cline",          "Cline",           ".cline/mcp.json",                "json", &generate_cline_config},
        {"opencode",       "OpenCode",        ".opencode/opencode.json",         "json", &generate_opencode_config},
        {"codex",          "Codex",           ".codex/config.toml",             "toml", &generate_codex_toml},
        {"trae",           "Trae",            ".trae/mcp.json",                 "json", &generate_trae_config},
        {"qoder",          "Qoder",           ".qoder/mcp.json",                "json", &generate_qoder_config},
        {"codebuddy",      "CodeBuddy",       ".codebuddy/mcp_settings.json",   "json", &generate_codebuddy_config},
        {"pi",             "Pi",              ".pi/settings.json",              "json", &generate_pi_config},
        {"openclaw",       "OpenClaw",        ".openclaw/openclaw.json",        "json", &generate_openclaw_config},
    };
    count = sizeof(entries) / sizeof(entries[0]);
    return entries;
}

} // namespace godot_mcp
