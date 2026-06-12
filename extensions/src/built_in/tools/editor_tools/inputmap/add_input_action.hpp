
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/input_map.hpp>

namespace godot_mcp {

class AddInputActionTool : public ITool {
public:
    String name() const override { return "add_input_action"; }
    String category() const override { return "editor_tools/inputmap"; }
    String brief() const override {
        return "Add a new InputMap action with deadzone";
    }
    String description() const override {
        return "Adds a new action to the InputMap with the specified deadzone. "
               "Fails if the action already exists.";
    }
    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Action name";
            props["action"] = p;
        }
        {
            Dictionary p;
            p["type"] = "number";
            p["description"] = "Deadzone value (0.0-1.0)";
            p["default"] = 0.2;
            props["deadzone"] = p;
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
        double deadzone = args_float(ctx.args, "deadzone", 0.2);

        if (action.is_empty()) {
            return ToolResult::err("BAD_PARAM", "action is required");
        }

        godot::StringName action_sn = godot::StringName(action);
        godot::TypedArray<godot::StringName> actions = im->get_actions();
        for (int i = 0; i < actions.size(); i++) {
            if (actions[i] == action_sn) {
                return ToolResult::err("ALREADY_EXISTS",
                    "Action already exists: " + action);
            }
        }

        im->add_action(action_sn, (real_t)deadzone);

        Dictionary data;
        data["action"] = action;
        data["deadzone"] = (real_t)deadzone;
        data["created"] = true;
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
