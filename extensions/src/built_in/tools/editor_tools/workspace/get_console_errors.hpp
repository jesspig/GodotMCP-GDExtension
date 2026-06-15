
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include "workspace_utils.hpp"

#include <godot_cpp/variant/dictionary.hpp>

namespace godot_mcp {

class GetConsoleErrorsTool : public ITool {
public:
    String name() const noexcept override { return "get_console_errors"; }
    String category() const noexcept override { return "editor_tools/workspace"; }
    String brief() const noexcept override { return String("Get editor console error messages"); }

    Dictionary build_input_schema() const override {
        Dictionary s; s["type"] = "object"; s["properties"] = Dictionary();
        return s;
    }

protected:
    Dictionary execute_impl(const ToolContext &) override {
        auto *rtl = find_console_rtl();
        if (!rtl) return ToolResult::err("NO_CONSOLE", "Console not found");

        PackedStringArray lines = rtl->get_text().split("\n", false);
        Array errors;
        for (int64_t i = 0; i < lines.size(); i++) {
            String l = lines[i];
            if (l.contains("MCP") || l.contains("mcp") || l.contains("godot_mcp")) {
                continue;
            }
            if (l.contains("ERROR") || l.contains("error") || l.contains("Error")) {
                errors.append(l);
            }
        }
        Dictionary d;
        d["count"] = static_cast<int64_t>(errors.size());
        d["errors"] = errors;
        return ToolResult::ok(d);
    }

};

} // namespace godot_mcp
