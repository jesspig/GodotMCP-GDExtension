
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/input_map.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_joypad_button.hpp>
#include <godot_cpp/classes/input_event_joypad_motion.hpp>

namespace godot_mcp {

class AddInputEventBindingTool : public ITool {
public:
    String name() const override { return "add_input_event_binding"; }
    String category() const override { return "editor_tools/inputmap"; }
    String brief() const override {
        return "Add an InputEvent binding to an existing action";
    }
    String description() const override {
        return "Adds an input event binding (key, mouse button, joypad button, or joypad axis) "
               "to an existing InputMap action.";
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
            p["type"] = "string";
            p["description"] = "Event type: key/mb/mouse_button/jb/joy_button/ja/joy_axis";
            props["event_type"] = p;
        }
        {
            Dictionary p;
            p["description"] = "Keycode integer or string (e.g. 87, \"KEY_W\") for key events";
            props["keycode"] = p;
        }
        {
            Dictionary p;
            p["type"] = "integer";
            p["description"] = "Button index for mouse/joy button events";
            props["button_index"] = p;
        }
        {
            Dictionary p;
            p["type"] = "integer";
            p["description"] = "Axis index for joy_axis events";
            props["axis"] = p;
        }
        {
            Dictionary p;
            p["type"] = "number";
            p["description"] = "Axis sign for joy_axis (1.0 or -1.0)";
            props["axis_sign"] = p;
        }
        {
            Dictionary p;
            p["type"] = "object";
            p["description"] = "Key modifiers {ctrl, shift, alt, meta} booleans";
            props["modifiers"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        {
            Array req;
            req.append("action");
            req.append("event_type");
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
        String event_type = args_string(ctx.args, "event_type").to_lower();

        if (action.is_empty()) {
            return ToolResult::err("BAD_PARAM", "action is required");
        }
        if (event_type.is_empty()) {
            return ToolResult::err("BAD_PARAM", "event_type is required");
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

        godot::Ref<godot::InputEvent> event;

        if (event_type == "key") {
            godot::InputEventKey *ev = memnew(godot::InputEventKey);

            int64_t keycode = args_int(ctx.args, "keycode", 0);
            if (keycode == 0) {
                String keycode_str = args_string(ctx.args, "keycode");
                if (!keycode_str.is_empty()) {
                    if (keycode_str.begins_with("KEY_")) {
                        Variant v = godot::Variant(godot::String("Key.") + keycode_str.substr(4, -1).capitalize().replace("_", ""));
                        keycode = (int64_t)v;
                    }
                    if (keycode == 0) {
                        keycode = keycode_str.to_int();
                    }
                }
            }
            if (keycode != 0) {
                ev->set_keycode(static_cast<godot::Key>(keycode));
            }

            if (ctx.args.has("modifiers") && ctx.args["modifiers"].get_type() == Variant::DICTIONARY) {
                Dictionary mods = ctx.args["modifiers"];
                if (args_bool(mods, "ctrl", false)) ev->set_ctrl_pressed(true);
                if (args_bool(mods, "shift", false)) ev->set_shift_pressed(true);
                if (args_bool(mods, "alt", false)) ev->set_alt_pressed(true);
                if (args_bool(mods, "meta", false)) ev->set_meta_pressed(true);
            }

            ev->set_pressed(true);
            event = godot::Ref<godot::InputEvent>(ev);

        } else if (event_type == "mouse_button" || event_type == "mb") {
            godot::InputEventMouseButton *ev = memnew(godot::InputEventMouseButton);
            int64_t btn = args_int(ctx.args, "button_index", 1);
            ev->set_button_index(static_cast<godot::MouseButton>(btn));
            ev->set_pressed(true);
            event = godot::Ref<godot::InputEvent>(ev);

        } else if (event_type == "joy_button" || event_type == "jb") {
            godot::InputEventJoypadButton *ev = memnew(godot::InputEventJoypadButton);
            int64_t btn = args_int(ctx.args, "button_index", 0);
            ev->set_button_index(static_cast<godot::JoyButton>(btn));
            ev->set_pressed(true);
            event = godot::Ref<godot::InputEvent>(ev);

        } else if (event_type == "joy_axis" || event_type == "ja") {
            godot::InputEventJoypadMotion *ev = memnew(godot::InputEventJoypadMotion);
            int64_t axis = args_int(ctx.args, "axis", 0);
            double axis_sign = args_float(ctx.args, "axis_sign", 1.0);
            ev->set_axis(static_cast<godot::JoyAxis>(axis));
            ev->set_axis_value((real_t)axis_sign);
            event = godot::Ref<godot::InputEvent>(ev);

        } else {
            return ToolResult::err("UNKNOWN_TYPE",
                "Unknown event_type: " + event_type);
        }

        if (event.is_null()) {
            return ToolResult::err("CREATE_FAILED", "Failed to create input event");
        }

        im->action_add_event(action_sn, event);

        Dictionary data;
        data["action"] = action;
        data["event_type"] = event_type;
        data["added"] = true;
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
