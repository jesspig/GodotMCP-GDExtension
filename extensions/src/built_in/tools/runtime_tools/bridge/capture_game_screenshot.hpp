
#pragma once

#include "built_in/cmd_utils/schema_builder.hpp"
#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "runtime/bridge_server.hpp"
#include "server/registry/handler_registry.hpp"

namespace godot_mcp {

class CaptureGameScreenshotTool : public ITool {
    HandlerRegistry *registry_ = nullptr;
public:
    String name() const noexcept override { return "capture_game_screenshot"; }
    String category() const noexcept override { return "runtime_tools/bridge"; }
    String brief() const noexcept override { return String("Capture the game viewport of a running game"); }
    String description() const override {
        return String("Captures a screenshot of the running game's viewport and returns base64-encoded image data. "
                              "format can be png (default) or jpg. "
                              "Returns immediately with a pending token; the actual response arrives via SSE. "
                              "This is the biggest UX improvement — no more 5-second editor freeze.");
    }
    void set_registry(HandlerRegistry *reg) override { registry_ = reg; }

    Dictionary build_input_schema() const override {
        return SchemaBuilder()
            .prop("format", "string", "Image format: png or jpg (default png)", "png")
            .prop("instance_id", "integer", "Game instance ID (default: first connected instance)", (int64_t)0)
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
        params["format"] = args_string(ctx.args, "format", "png");
        Dictionary raw = bridge->send_command_sync(instance_id, "screenshot", params);
        if (raw.has("pending")) return raw;
        return RuntimeBridgeServer::make_response(raw);
    }
};

} // namespace godot_mcp
