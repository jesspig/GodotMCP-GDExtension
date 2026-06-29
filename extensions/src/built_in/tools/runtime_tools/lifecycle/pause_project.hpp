
#pragma once

#include "built_in/cmd_utils/schema_builder.hpp"
#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "server/registry/handler_registry.hpp"
#include "runtime/bridge_server.hpp"

namespace godot_mcp {

class PauseProjectTool : public ITool {
    HandlerRegistry *registry_ = nullptr;
public:
    String name() const noexcept override { return "pause_project"; }
    String category() const noexcept override { return "runtime_tools/lifecycle"; }
    String brief() const noexcept override { return String("Pause or resume the running project"); }
    String description() const override {
        return String("Pauses or resumes the running project. When paused, the game scene's _process and _physics_process stop processing, "
                             "but the engine continues running. Equivalent to calling SceneTree.set_pause(). "
                             "This feature requires the game to be connected via the runtime bridge (:9601). "
                             "Returns an error if the game is not running or the bridge is not connected.");
    }
    void set_registry(HandlerRegistry *reg) override { registry_ = reg; }

    Dictionary build_input_schema() const override {
        return SchemaBuilder()
            .prop("paused", "boolean", "true to pause, false to resume")
            .required({"paused"})
            .build();
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        RuntimeBridgeServer *bridge = registry_ ? registry_->get_runtime_bridge_server() : nullptr;
        if (!bridge || !bridge->is_connected()) {
            return ToolResult::err("GAME_NOT_RUNNING", "Game not running or bridge not connected");
        }
        bool paused = args_bool(ctx.args, "paused");
        Dictionary params;
        params["paused"] = paused;
        int instance_id = bridge->get_default_instance_id();
        if (instance_id < 0) {
            return ToolResult::err("GAME_NOT_RUNNING", "No game instance connected");
        }
        Dictionary raw = bridge->send_command_sync(instance_id, "set_pause", params);
        if (raw.is_empty()) {
            return ToolResult::err("BRIDGE_TIMEOUT", "Bridge command timed out");
        }
        return RuntimeBridgeServer::make_response(raw);
    }
};

} // namespace godot_mcp

