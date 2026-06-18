
#pragma once

#include "built_in/cmd_utils/schema_builder.hpp"
#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "server/registry/handler_registry.hpp"
#include "runtime/bridge.hpp"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/time.hpp>

namespace godot_mcp {

class WaitForBridgeTool : public ITool {
    HandlerRegistry *registry_ = nullptr;
public:
    String name() const noexcept override { return "wait_for_bridge"; }
    String category() const noexcept override { return "runtime_tools/bridge"; }
    String brief() const noexcept override { return String("Wait for runtime bridge to connect"); }
    String description() const override {
        return String("Polls the runtime bridge connection until the game process connects back. "
                              "Call after run_project / run_current_scene / run_specific_scene to block "
                              "until the bridge is ready for use. Returns immediately if already connected.");
    }
    void set_registry(HandlerRegistry *reg) override { registry_ = reg; }

    Dictionary build_input_schema() const override {
        return SchemaBuilder()
            .prop("timeout_ms", "integer", "Max wait time in ms", (int64_t)8000)
            .prop("interval_ms", "integer", "Poll interval in ms", (int64_t)200)
            .build();
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        RuntimeBridge *bridge = registry_ ? registry_->get_runtime_bridge() : nullptr;
        if (!bridge) {
            return ToolResult::err("BRIDGE_UNAVAILABLE", "Runtime bridge not available in this context");
        }

        if (bridge->is_connected()) {
            Dictionary data;
            data["message"] = "Bridge already connected";
            return ToolResult::ok(data);
        }

        int timeout_ms = args_int(ctx.args, "timeout_ms", 8000);
        int interval_ms = args_int(ctx.args, "interval_ms", 200);

        auto *ei = EditorInterface::get_singleton();

        uint64_t start = Time::get_singleton()->get_ticks_msec();
        while (Time::get_singleton()->get_ticks_msec() - start < (uint64_t)timeout_ms) {
            if (!bridge->is_connected()) {
                if (ei && ei->is_playing_scene()) {
                    bridge->connect();
                }
            }
            bridge->poll();
            if (bridge->is_connected()) {
                Dictionary data;
                data["message"] = "Bridge connected";
                return ToolResult::ok(data);
            }
            OS::get_singleton()->delay_msec(interval_ms);
        }

        return ToolResult::err("BRIDGE_TIMEOUT",
            "Bridge not connected within " + String::num_int64(timeout_ms) + "ms");
    }
};

} // namespace godot_mcp
