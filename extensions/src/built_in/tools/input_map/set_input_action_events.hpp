// @tool register
// @source builtin
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/tools/input_map/input_map_helpers.hpp"
#include <godot_cpp/classes/input_map.hpp>
#include <godot_cpp/classes/project_settings.hpp>

namespace godot_mcp {

class SetInputActionEventsTool : public ITool {
public:
    String name() const override { return "set_input_action_events"; }
    String category() const override { return "input_map"; }
    String brief() const override { return "Set events for an input action"; }
    String description() const override { return "Set events for an input action"; }
    Dictionary input_schema() const override {
        Dictionary s; s["type"] = "object"; Dictionary p;
        p["name"] = [](){Dictionary d;d["type"]="string";d["description"]="Action name";return d;}();
        p["mode"] = [](){Dictionary d;d["type"]="string";d["description"]="replace | add | clear";return d;}();
        p["events"] = [](){Dictionary d;d["type"]="array";d["description"]="Event descriptors";return d;}();
        s["properties"] = p; Array r; r.push_back("name"); s["required"] = r; return s;
    }
protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String name = args_string(ctx.args, "name");
        if (name.is_empty()) return ToolResult::err("MISSING_PARAM", "missing 'name'");
        String mode = args_string(ctx.args, "mode", "replace");
        String key = "input/" + name;
        if (!ProjectSettings::get_singleton()->has_setting(key))
            return ToolResult::err("NOT_FOUND", "action '" + name + "' not found");
        Variant existing = ProjectSettings::get_singleton()->get_setting(key);
        Dictionary action_dict = existing;
        if (mode == "clear") { action_dict["events"] = Array(); }
        else if (mode == "replace" || mode == "add") {
            Array new_events;
            if (mode == "add" && action_dict.has("events")) { Array old = action_dict["events"]; for (int i = 0; i < old.size(); i++) new_events.append(old[i]); }
            if (ctx.args.has("events") && ctx.args["events"].get_type() == Variant::ARRAY) {
                Array evs = ctx.args["events"];
                for (int i = 0; i < evs.size(); i++) {
                    Variant ev = deserialize_event(evs[i]);
                    if (ev.get_type() != Variant::NIL) new_events.append(ev);
                }
            }
            action_dict["events"] = new_events;
        } else return ToolResult::err("INVALID_PARAM", "mode must be 'replace', 'add', or 'clear'");
        ProjectSettings::get_singleton()->set_setting(key, action_dict);
        Dictionary d; d["name"] = name;
        if (ProjectSettings::get_singleton()->save() != OK) d["warning"] = "disk write failed";
        InputMap::get_singleton()->load_from_project_settings();
        return ToolResult::ok(d);
    }
};

} // namespace godot_mcp
