
#pragma once

#include "built_in/cmd_utils/schema_builder.hpp"
#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "runtime/bridge_server.hpp"
#include "server/registry/handler_registry.hpp"

namespace godot_mcp {

class GetGameNodePropertyTool : public ITool {
    HandlerRegistry *registry_ = nullptr;
public:
    String name() const noexcept override { return "get_game_node_property"; }
    String category() const noexcept override { return "runtime_tools/bridge"; }
    String brief() const noexcept override { return String("Read a running game node property"); }
    String description() const override {
        return String("Reads a property value from a node in the running game. "
                              "node_path is the node path (e.g. /root/Main/Player), "
                              "property is the property name (e.g. position, rotation). "
                              "Returns immediately with a pending token; the actual response arrives via SSE.");
    }
    void set_registry(HandlerRegistry *reg) override { registry_ = reg; }

    Dictionary build_input_schema() const override {
        return SchemaBuilder()
            .prop("node_path", "string", "Node path, e.g. /root/Main/Player")
            .prop("property", "string", "Property name, e.g. position")
            .prop("instance_id", "integer", "Game instance ID (default: first connected instance)", (int64_t)0)
            .required({"node_path", "property"})
            .build();
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
        Dictionary raw = bridge->send_command_sync(instance_id, "get_property", params);
        if (raw.has("pending")) return raw;
        return RuntimeBridgeServer::make_response(raw);
    }
};

} // namespace godot_mcp
