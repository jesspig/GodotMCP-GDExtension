
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include "workspace_utils.hpp"

#include <godot_cpp/variant/dictionary.hpp>

namespace godot_mcp {

class DebuggerStepOutTool : public ITool {
public:
    String name() const noexcept override { return "debugger_step_out"; }
    String category() const noexcept override { return "editor_tools/workspace"; }
    String brief() const noexcept override { return String("Debugger step out"); }
    String description() const override {
        return String("Steps out of the current function (Godot has no direct step_out command, "
                      "uses debug_next as a substitute).");
    }

    Dictionary build_input_schema() const override {
        Dictionary s; s["type"] = "object"; s["properties"] = Dictionary();
        return s;
    }

protected:
    Dictionary execute_impl(const ToolContext &) override {
        Object *dbg = find_debugger();
        if (!dbg) return ToolResult::err("NO_DEBUGGER", "EditorDebuggerNode not found");
        dbg->call("debug_next");
        Dictionary d;
        d["action"] = "step_out";
        d["performed"] = true;
        return ToolResult::ok(d);
    }

};

} // namespace godot_mcp
