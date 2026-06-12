
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/input_map.hpp>

namespace godot_mcp {

class RemoveInputActionTool : public ITool {
public:
    String name() const override { return "remove_input_action"; }
    String category() const override { return "editor_tools/inputmap"; }
    String brief() const override {
        return "Remove an InputMap action";
    }
    String description() const override {
        return "Removes an action and all its event bindings from the InputMap. "
               "Fails if the action does not exist.";
    }
    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Action name to remove";
            props["action"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        {
            Array req;
            req.append("action");
            s["required"] = req;
        }
        return s;
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        godot::InputMap *im = godot::InputMap::get_singleton();
        if (!im) {
            return ToolResult::err("NO_INPUT_MAP", "InputMap not available");
        }

        String action = args_string(ctx.args, "action");

        if (action.is_empty()) {
            return ToolResult::err("BAD_PARAM", "action is required");
        }

        godot::StringName action_sn = godot::StringName(action);

        godot::TypedArray<godot::StringName> actions = im->get_actions();
        bool found = false;
        for (int i = 0; i < actions.size(); i++) {
            if (actions[i] == action_sn) {
                found = true;
                break;
            }
        }
        if (!found) {
            return ToolResult::err("NOT_FOUND",
                "Action does not exist: " + action);
        }

        im->erase_action(action_sn);

        Dictionary data;
        data["action"] = action;
        data["removed"] = true;
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
