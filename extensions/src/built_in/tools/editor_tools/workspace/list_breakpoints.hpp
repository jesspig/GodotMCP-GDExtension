
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/script_editor.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace godot_mcp {

class ListBreakpointsTool : public ITool {
public:
    String name() const override { return "list_breakpoints"; }
    String category() const override { return "editor_tools/workspace"; }
    String brief() const override { return String("List all breakpoints"); }
    String description() const override { return brief(); }

    Dictionary input_schema() const override {
        Dictionary s; s["type"] = "object"; s["properties"] = Dictionary();
        return s;
    }

protected:
    Dictionary execute_impl(const ToolContext &) override {
        Array breakpoints;
        Array bp_data;

        EditorInterface *ei = EditorInterface::get_singleton();
        if (!ei) {
            Dictionary data;
            data["breakpoints"] = Array();
            data["count"] = (int64_t)0;
            return ToolResult::ok(data);
        }

        ScriptEditor *se = ei->get_script_editor();
        if (se) {
            breakpoints = se->call("get_breakpoints");
        }

        // Fallback: try finding ScriptEditor via scene tree
        if (breakpoints.size() == 0) {
            Control *base = ei->get_base_control();
            if (base) {
                Array nodes = base->find_children("*", "ScriptEditor", true, false);
                if (nodes.size() > 0) {
                    Object *se_obj = Object::cast_to<Node>(nodes[0]);
                    if (se_obj) {
                        breakpoints = se_obj->call("get_breakpoints");
                    }
                }
            }
        }

        // breakpoints is an Array of Dictionaries {script, line}
        for (int64_t i = 0; i < breakpoints.size(); i++) {
            Variant v = breakpoints[i];
            if (v.get_type() == Variant::DICTIONARY) {
                Dictionary bp = v;
                Dictionary entry;
                entry["script_path"] = bp.get("script", "");
                entry["line"] = bp.get("line", (int64_t)0);
                bp_data.append(entry);
            }
        }

        Dictionary data;
        data["breakpoints"] = bp_data;
        data["count"] = (int64_t)bp_data.size();
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
