
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "server/registry/handler_registry.hpp"
#include "runtime/bridge.hpp"

namespace godot_mcp {

class SetGameNodePropertyTool : public ITool {
    HandlerRegistry *registry_ = nullptr;
public:
    String name() const noexcept override { return "set_game_node_property"; }
    String category() const noexcept override { return "runtime_tools/bridge"; }
    String brief() const noexcept override { return String("Modify a running game node property"); }
    String description() const override {
        return String("Modifies a property value on a node in the running game. "
                             "node_path is the node path, property is the property name, "
                             "value is the new value to set.");
    }
    void set_registry(HandlerRegistry *reg) override { registry_ = reg; }

    Dictionary build_input_schema() const override {
        Dictionary p;
        {
            Dictionary d;
            d["type"] = "string";
            d["description"] = String("Node path, e.g. /root/Main/Player");
            p["node_path"] = d;
        }
        {
            Dictionary d;
            d["type"] = "string";
            d["description"] = String("Property name, e.g. position");
            p["property"] = d;
        }
        {
            Dictionary d;
            d["description"] = String("New value to set");
            p["value"] = d;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = p;
        s["required"] = Array::make("node_path", "property", "value");
        return s;
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        RuntimeBridge *bridge = registry_ ? registry_->get_runtime_bridge() : nullptr;
        if (!bridge || !bridge->is_connected()) {
            return ToolResult::err("GAME_NOT_RUNNING", "Game not running");
        }
        Dictionary params;
        params["node_path"] = args_string(ctx.args, "node_path");
        params["property"] = args_string(ctx.args, "property");
        params["value"] = ctx.args.get("value", Variant());
        return RuntimeBridge::make_response(bridge->send_command("set_property", params));
    }
};

} // namespace godot_mcp

