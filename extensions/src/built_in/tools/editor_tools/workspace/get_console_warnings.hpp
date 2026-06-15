
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include "workspace_utils.hpp"

#include <godot_cpp/variant/dictionary.hpp>

namespace godot_mcp {

class GetConsoleWarningsTool : public ITool {
public:
    String name() const override { return "get_console_warnings"; }
    String category() const override { return "editor_tools/workspace"; }
    String brief() const override { return String("Get editor console warning messages"); }
    String description() const override { return brief(); }

    Dictionary build_input_schema() const override {
        Dictionary s; s["type"] = "object"; s["properties"] = Dictionary();
        return s;
    }

protected:
    Dictionary execute_impl(const ToolContext &) override {
        godot::RichTextLabel *rtl = find_console_rtl();
        if (!rtl) return ToolResult::err("NO_CONSOLE", "Console not found");

        PackedStringArray lines = rtl->get_text().split("\n", false);
        Array warnings;
        for (int64_t i = 0; i < lines.size(); i++) {
            String l = lines[i];
            if (l.contains("MCP") || l.contains("mcp") || l.contains("godot_mcp")) {
                continue;
            }
            if (l.contains("WARNING") || l.contains("warning") || l.contains("Warning")) {
                warnings.append(l);
            }
        }
        Dictionary d;
        d["count"] = static_cast<int64_t>(warnings.size());
        d["warnings"] = warnings;
        return ToolResult::ok(d);
    }

};

} // namespace godot_mcp
