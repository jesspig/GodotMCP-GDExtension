
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "server/registry/handler_registry.hpp"
#include "runtime/bridge.hpp"

namespace godot_mcp {

class SimulateGameInputTool : public ITool {
    HandlerRegistry *registry_ = nullptr;
public:
    String name() const override { return "simulate_game_input"; }
    String category() const override { return "runtime_tools/bridge"; }
    String brief() const override { return String("Simulate input in a running game"); }
    String description() const override {
        return String("Sends keyboard, mouse or Action input events to the running game. "
                             "actions is an array of input actions, each containing type (key/mouse_button/mouse_motion/action) and corresponding parameters.\n"
                             "Examples:\n"
                             "- Key: {\"type\": \"key\", \"key\": \"W\", \"pressed\": true}\n"
                             "- Mouse click: {\"type\": \"mouse_button\", \"button\": \"left\", \"x\": 100, \"y\": 200}\n"
                             "- Action: {\"type\": \"action\", \"action\": \"ui_accept\", \"pressed\": true}");
    }
    void set_registry(HandlerRegistry *reg) override { registry_ = reg; }

    Dictionary input_schema() const override {
        Dictionary p;
        {
            Dictionary item_schema;
            item_schema["type"] = "object";
            Dictionary item_props;
            {
                Dictionary tp;
                tp["type"] = "string";
                tp["description"] = "Action type: key / mouse_button / mouse_motion / action";
                item_props["type"] = tp;
            }
            {
                Dictionary kp;
                kp["type"] = "string";
                kp["description"] = "Key name (required for key type), e.g. W, Space, Escape";
                item_props["key"] = kp;
            }
            {
                Dictionary bp;
                bp["type"] = "string";
                bp["description"] = "Mouse button (for mouse_button type): left / right / middle";
                item_props["button"] = bp;
            }
            {
                Dictionary ap;
                ap["type"] = "string";
                ap["description"] = "Action name (required for action type)";
                item_props["action"] = ap;
            }
            item_schema["properties"] = item_props;
            item_schema["required"] = Array::make("type");

            Dictionary d;
            d["type"] = "array";
            d["description"] = String("Array of input actions");
            d["items"] = item_schema;
            p["actions"] = d;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = p;
        s["required"] = Array::make("actions");
        return s;
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        RuntimeBridge *bridge = registry_ ? registry_->get_runtime_bridge() : nullptr;
        if (!bridge || !bridge->is_connected()) {
            return ToolResult::err("GAME_NOT_RUNNING", "Game not running");
        }
        Dictionary params;
        params["actions"] = ctx.args.get("actions", Array());
        return RuntimeBridge::make_response(bridge->send_command("simulate_input", params));
    }
};

} // namespace godot_mcp

