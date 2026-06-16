#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include "workspace_utils.hpp"

#include <godot_cpp/variant/dictionary.hpp>

namespace godot_mcp {

class GetDebuggerErrorsTool : public ITool {
public:
    String name() const noexcept override { return "get_debugger_errors"; }
    String category() const noexcept override { return "editor_tools/workspace"; }
    String brief() const noexcept override { return String("Get debugger error and warning counts"); }
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
            d["error_count"] = static_cast<int64_t>(active->call("get_error_count"));
            d["warning_count"] = static_cast<int64_t>(active->call("get_warning_count"));
        } else {
            d["error_count"] = static_cast<int64_t>(0);
            d["warning_count"] = static_cast<int64_t>(0);
        }
        return ToolResult::ok(d);
    }

};

} // namespace godot_mcp
