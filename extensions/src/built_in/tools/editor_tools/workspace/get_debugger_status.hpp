#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include "workspace_utils.hpp"

#include <godot_cpp/variant/dictionary.hpp>

namespace godot_mcp {

class GetDebuggerStatusTool : public ITool {
public:
    String name() const noexcept override { return "get_debugger_status"; }
    String category() const noexcept override { return "editor_tools/workspace"; }
    String brief() const noexcept override { return String("Get debugger running status"); }
    String description() const override { return brief(); }

    Dictionary build_input_schema() const override {
        Dictionary s; s["type"] = "object"; s["properties"] = Dictionary();
        return s;
    }

protected:
    Dictionary execute_impl(const ToolContext &) override {
        Object *dbg = find_debugger();
        if (!dbg) return ToolResult::err("NO_DEBUGGER", "EditorDebuggerNode not found");

        Object *active = dbg->call("get_current_debugger");
        Dictionary d;
        if (active) {
            d["is_active"] = true;
            d["is_breaked"] = static_cast<bool>(active->call("is_breaked"));
            d["is_debuggable"] = static_cast<bool>(active->call("is_debuggable"));
            d["is_session_active"] = static_cast<bool>(active->call("is_session_active"));
        } else {
            d["is_active"] = false;
        }
        return ToolResult::ok(d);
    }

};

} // namespace godot_mcp
