
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "runtime/bridge_server.hpp"
#include "server/registry/handler_registry.hpp"

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
                              "value is the new value to set. "
                              "Returns immediately with a pending token; the actual response arrives via SSE.");
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
        {
            Dictionary d;
            d["type"] = "integer";
            d["description"] = String("Game instance ID (default: first connected instance)");
            p["instance_id"] = d;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = p;
        s["required"] = Array::make("node_path", "property", "value");
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
        params["node_path"] = args_string(ctx.args, "node_path");
        params["property"] = args_string(ctx.args, "property");
        params["value"] = ctx.args.get("value", Variant());
        Dictionary raw = bridge->send_command_sync(instance_id, "set_property", params);
        if (raw.has("pending")) return raw;
        return RuntimeBridgeServer::make_response(raw);
    }
};

} // namespace godot_mcp
