
#pragma once

#include "built_in/cmd_utils/schema_builder.hpp"
#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "runtime/bridge_server.hpp"
#include "server/mcp/mcp_handler.hpp"
#include "server/registry/handler_registry.hpp"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/time.hpp>

namespace godot_mcp {

class WaitForBridgeTool : public ITool {
    HandlerRegistry *registry_ = nullptr;
public:
    String name() const noexcept override { return "wait_for_bridge"; }
    String category() const noexcept override { return "runtime_tools/bridge"; }
    String brief() const noexcept override { return String("Wait for runtime bridge to connect"); }
    String description() const override {
        return String("Waits for a game instance to connect to the bridge server. "
                      "Call after run_project / run_current_scene / run_specific_scene. "
                      "Returns immediately if already connected. Non-blocking - no editor freeze. "
                      "Response arrives via SSE when a game instance connects or timeout expires. "
                      "Optional instance_id: wait for a specific instance; omit to wait for any.");
    }
    void set_registry(HandlerRegistry *reg) override { registry_ = reg; }

    Dictionary build_input_schema() const override {
        return SchemaBuilder()
            .prop("timeout_ms", "integer", "Max wait time in ms", (int64_t)8000)
            .prop("instance_id", "integer", "Optional: wait for a specific instance ID (0 = any)", (int64_t)0)
            .build();
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        RuntimeBridgeServer *bridge = registry_ ? registry_->get_runtime_bridge_server() : nullptr;
        if (!bridge) {
            return ToolResult::err("BRIDGE_UNAVAILABLE", "Runtime bridge not available in this context");
        }

        int instance_id = args_int(ctx.args, "instance_id", 0);

        if (instance_id > 0 && bridge->has_instance(instance_id)) {
            Dictionary data;
            data["message"] = "Specified instance already connected";
            data["instance_id"] = instance_id;
            data["instances"] = bridge->get_connected_instances();
            return ToolResult::ok(data);
        }

        if (instance_id <= 0 && bridge->has_instances()) {
            Dictionary data;
            data["message"] = "Bridge already connected";
            data["instances"] = bridge->get_connected_instances();
            return ToolResult::ok(data);
        }

        int timeout = args_int(ctx.args, "timeout_ms", 8000);
        bridge->start_watcher(ctx.jsonrpc_id, timeout);
        return Dictionary();  // defer to SSE
    }
};

} // namespace godot_mcp
