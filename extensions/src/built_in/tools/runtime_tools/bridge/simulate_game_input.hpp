
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "runtime/bridge_server.hpp"
#include "server/registry/handler_registry.hpp"

namespace godot_mcp {

class SimulateGameInputTool : public ITool {
    HandlerRegistry *registry_ = nullptr;
public:
    String name() const noexcept override { return "simulate_game_input"; }
    String category() const noexcept override { return "runtime_tools/bridge"; }
    String brief() const noexcept override { return String("Simulate input in a running game"); }
    String description() const override {
        return String("Sends keyboard, mouse or Action input events to the running game. "
                              "actions is an array of input actions, each containing type (key/mouse_button/mouse_motion/action) and corresponding parameters.\n"
                              "Returns immediately with a pending token; the actual response arrives via SSE.\n"
                              "Examples:\n"
                              "- Key: {\"type\": \"key\", \"key\": \"W\", \"pressed\": true}\n"
                              "- Mouse click: {\"type\": \"mouse_button\", \"button\": \"left\", \"x\": 100, \"y\": 200}\n"
                              "- Action: {\"type\": \"action\", \"action\": \"ui_accept\", \"pressed\": true}");
    }
    void set_registry(HandlerRegistry *reg) override { registry_ = reg; }

    Dictionary build_input_schema() const override {
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
        {
            Dictionary d;
            d["type"] = "integer";
            d["description"] = String("Game instance ID (default: first connected instance)");
            p["instance_id"] = d;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = p;
        s["required"] = Array::make("actions");
        return s;
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        RuntimeBridgeServer *bridge = registry_ ? registry_->get_runtime_bridge_server() : nullptr;
        if (!bridge || !bridge->has_instances()) {
            return ToolResult::err("GAME_NOT_RUNNING", "No game instances connected");
        }
        int instance_id = args_int(ctx.args, "instance_id", bridge->get_default_instance_id());
        if (!bridge->has_instance(instance_id)) {
            return ToolResult::err("INSTANCE_NOT_FOUND",
                                   String("Game instance ") + String::num_int64(instance_id) + String(" not found"));
        }
        Dictionary params;
        params["actions"] = ctx.args.get("actions", Array());
        Dictionary raw = bridge->send_command_sync(instance_id, "simulate_input", params);
        if (raw.has("pending")) return raw;
        return RuntimeBridgeServer::make_response(raw);
    }
};

} // namespace godot_mcp
