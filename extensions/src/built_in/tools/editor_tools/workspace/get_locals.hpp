
#pragma once

#include "built_in/cmd_utils/schema_builder.hpp"
#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include "workspace_utils.hpp"

#include <godot_cpp/variant/dictionary.hpp>

namespace godot_mcp {

class GetLocalsTool : public ITool {
public:
    String name() const noexcept override { return "get_locals"; }
    String category() const noexcept override { return "editor_tools/workspace"; }
    String brief() const noexcept override { return String("Get local variables of current stack frame"); }

    Dictionary build_input_schema() const override {
        return SchemaBuilder()
            .prop("frame", "integer", String("Stack frame index (default 0)"), 0)
            .build();
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        int64_t frame = args_int(ctx.args, "frame", 0);

        Object *debugger = find_debugger();
        if (!debugger)
            return ToolResult::err("NO_DEBUGGER", "EditorDebuggerNode not found");

        Object *active_dbg = debugger->call("get_current_debugger");
        if (!active_dbg) {
            Dictionary data;
            data["breaked"] = false;
            data["locals"] = Array();
            data["frame"] = frame;
            return ToolResult::ok(data);
        }

        bool is_breaked = static_cast<bool>(active_dbg->call("is_breaked"));
        if (!is_breaked) {
            Dictionary data;
            data["breaked"] = false;
            data["locals"] = Array();
            data["frame"] = frame;
            return ToolResult::ok(data);
        }

        Array locals;
        int64_t var_count = static_cast<int64_t>(active_dbg->call("get_stack_variable_count", frame));
        for (int64_t i = 0; i < var_count; i++) {
            Dictionary var_info = active_dbg->call("get_stack_variable", frame, i);
            Dictionary entry;
            entry["name"] = var_info.get("name", "");
            entry["value"] = var_info.get("value", Variant());
            entry["type"] = var_info.get("type", "");
            locals.append(entry);
        }

        Dictionary data;
        data["breaked"] = true;
        data["locals"] = locals;
        data["frame"] = frame;
        return ToolResult::ok(data);
    }

};

} // namespace godot_mcp
