
#pragma once

#include "built_in/tool_base.hpp"
#include "runtime/bridge_server.hpp"
#include "server/registry/handler_registry.hpp"

namespace godot_mcp {

class ListGameInstancesTool : public ITool {
    HandlerRegistry *registry_ = nullptr;
public:
    String name() const noexcept override { return "list_game_instances"; }
    String category() const noexcept override { return "runtime_tools/bridge"; }
    String brief() const noexcept override { return String("List all connected game instances"); }
    String description() const override {
        return String("Returns an array of all currently connected game instances "
                      "with their IDs, connection timestamps, and uptime.");
    }
    void set_registry(HandlerRegistry *reg) override { registry_ = reg; }

    bool is_meta() const noexcept override { return false; }

protected:
    Dictionary execute_impl(const ToolContext &) override {
        RuntimeBridgeServer *bridge = registry_ ? registry_->get_runtime_bridge_server() : nullptr;
        if (!bridge) {
            return ToolResult::err("BRIDGE_UNAVAILABLE", "Runtime bridge not available");
        }
        Array instances = bridge->get_connected_instances();
        Dictionary data;
        data["instances"] = instances;
        data["count"] = bridge->instance_count();
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
