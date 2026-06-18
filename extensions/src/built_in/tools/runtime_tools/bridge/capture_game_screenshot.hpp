
#pragma once

#include "built_in/cmd_utils/schema_builder.hpp"
#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "server/registry/handler_registry.hpp"
#include "runtime/bridge.hpp"

namespace godot_mcp {

class CaptureGameScreenshotTool : public ITool {
    HandlerRegistry *registry_ = nullptr;
public:
    String name() const noexcept override { return "capture_game_screenshot"; }
    String category() const noexcept override { return "runtime_tools/bridge"; }
    String brief() const noexcept override { return String("Capture the game viewport of a running game"); }
    String description() const override {
        return String("Captures a screenshot of the running game's viewport and returns base64-encoded image data. "
                             "format can be png (default) or jpg.");
    }
    void set_registry(HandlerRegistry *reg) override { registry_ = reg; }

    Dictionary build_input_schema() const override {
        return SchemaBuilder()
            .prop("format", "string", "Image format: png or jpg (default png)", "png")
            .prop("timeout_ms", "integer", "Response timeout in ms", (int64_t)5000)
            .build();
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        RuntimeBridge *bridge = registry_ ? registry_->get_runtime_bridge() : nullptr;
        if (!bridge || !bridge->is_connected()) {
            return ToolResult::err("GAME_NOT_RUNNING", "Game not running");
        }
        Dictionary params;
        params["format"] = args_string(ctx.args, "format", "png");
        int timeout = args_int(ctx.args, "timeout_ms", 5000);
        return RuntimeBridge::make_response(bridge->send_command("screenshot", params, timeout));
    }
};

} // namespace godot_mcp

