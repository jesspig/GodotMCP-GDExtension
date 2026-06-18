
#pragma once

#include "built_in/cmd_utils/schema_builder.hpp"
#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "server/registry/handler_registry.hpp"
#include "runtime/bridge.hpp"

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
                             "property is the property name (e.g. position, rotation).");
    }
    void set_registry(HandlerRegistry *reg) override { registry_ = reg; }

    Dictionary build_input_schema() const override {
        return SchemaBuilder()
            .prop("node_path", "string", "Node path, e.g. /root/Main/Player")
            .prop("property", "string", "Property name, e.g. position")
            .prop("timeout_ms", "integer", "Response timeout in ms", (int64_t)100)
            .required({"node_path", "property"})
            .build();
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
        int timeout = args_int(ctx.args, "timeout_ms", 100);
        return RuntimeBridge::make_response(bridge->send_command("get_property", params, timeout));
    }
};

} // namespace godot_mcp

