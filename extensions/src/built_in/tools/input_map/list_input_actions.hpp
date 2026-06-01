// @tool register
// @source builtin
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/tools/input_map/input_map_helpers.hpp"
#include <godot_cpp/classes/input_map.hpp>

namespace godot_mcp {

class ListInputActionsTool : public ITool {
public:
    String name() const override { return "list_input_actions"; }
    String category() const override { return "input_map"; }
    String brief() const override { return "List input actions"; }
    String description() const override { return "List input actions"; }
    Dictionary input_schema() const override {
        Dictionary s; s["type"] = "object"; Dictionary p;
        p["include_builtin"] = [](){Dictionary d;d["type"]="boolean";d["description"]="Include built-in actions";return d;}();
        s["properties"] = p; return s;
    }
protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        bool builtin = args_bool(ctx.args, "include_builtin", false);
        InputMap *im = InputMap::get_singleton(); im->load_from_project_settings();
        Array acts = im->get_actions();
        Array out;
        for (int i = 0; i < acts.size(); i++) {
            String name = (String)acts[i];
            if (!builtin && (name.begins_with("ui_") || name.begins_with("editor/") ||
                name.begins_with("spatial_editor/") || name.begins_with("canvas_editor/") ||
                name.begins_with("text_editor/") || name.begins_with("script_editor/"))) continue;
            float dz = im->action_get_deadzone(name);
            Array evs_arr = im->action_get_events(name);
            Array evs_json;
            for (int j = 0; j < evs_arr.size(); j++) {
                Ref<InputEvent> ev = evs_arr[j]; evs_json.append(serialize_event(ev));
            }
            Dictionary e; e["name"] = name; e["deadzone"] = dz; e["events"] = evs_json;
            out.append(e);
        }
        Dictionary d; d["actions"] = out; d["count"] = (int64_t)out.size();
        return ToolResult::ok(d);
    }
};

} // namespace godot_mcp
