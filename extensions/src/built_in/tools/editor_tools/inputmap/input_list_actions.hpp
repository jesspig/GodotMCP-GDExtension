// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/input_map.hpp>
#include <godot_cpp/classes/input_event.hpp>

namespace godot_mcp {

class InputListActionsTool : public ITool {
public:
    String name() const override { return "input_list_actions"; }
    String category() const override { return "editor_tools/inputmap"; }
    String brief() const override {
        return "List all input actions and their events";
    }
    String description() const override {
        return "List all registered input actions from the InputMap, "
               "including their bound events and deadzone values.";
    }
    Dictionary input_schema() const override {
        Dictionary s;
        s["type"] = "object";
        s["properties"] = Dictionary();
        return s;
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        InputMap *im = InputMap::get_singleton();
        if (!im) {
            return ToolResult::err("NO_INPUT_MAP", "InputMap not available");
        }

        TypedArray<StringName> actions = im->get_actions();
        Array results;

        for (int i = 0; i < actions.size(); i++) {
            StringName action = actions[i];
            Array events = im->action_get_events(action);

            Dictionary entry;
            entry["name"] = action;
            entry["deadzone"] = im->action_get_deadzone(action);
            entry["event_count"] = (int64_t)events.size();

            Array event_list;
            for (int e = 0; e < events.size(); e++) {
                Ref<InputEvent> event = events[e];
                Dictionary ev_entry;
                ev_entry["type"] = event->get_class();
                ev_entry["as_text"] = event->call("as_text");

                if (event->is_action(action)) {
                    ev_entry["matched_action"] = true;
                }

                Array props = event->get_property_list();
                Dictionary prop_values;
                for (int p = 0; p < props.size(); p++) {
                    Dictionary prop = props[p];
                    String prop_name = prop["name"];
                    Variant val = event->get(prop_name);
                    prop_values[prop_name] = variant_to_json(val);
                }
                ev_entry["properties"] = prop_values;

                event_list.append(ev_entry);
            }
            entry["events"] = event_list;
            results.append(entry);
        }

        Dictionary data;
        data["actions"] = results;
        data["count"] = (int64_t)results.size();
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
