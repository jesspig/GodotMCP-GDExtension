
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
                      "max_width and max_height constrain the image size (preserving aspect ratio). "
                      "quality controls JPG compression (0.0-1.0, default 0.85). "
                      "Returns both legacy 'data' field and MCP 'ImageContent' type (type=image).");
    }
    void set_registry(HandlerRegistry *reg) override { registry_ = reg; }

    Dictionary build_input_schema() const override {
        return SchemaBuilder()
            .prop("format", "string", "Image format: png or jpg (default png)", "png")
            .prop("max_width", "integer", "Maximum width in pixels (preserves aspect ratio)", (int64_t)0)
            .prop("max_height", "integer", "Maximum height in pixels (preserves aspect ratio)", (int64_t)0)
            .prop("quality", "number", "JPG quality 0.0-1.0 (default 0.85, ignored for PNG)", 0.85)
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
        int64_t max_w = args_int(ctx.args, "max_width", 0);
        int64_t max_h = args_int(ctx.args, "max_height", 0);
        if (max_w > 0) params["max_width"] = max_w;
        if (max_h > 0) params["max_height"] = max_h;
        double quality = args_float(ctx.args, "quality", 0.85);
        if (quality > 0.0 && quality <= 1.0) params["quality"] = quality;

        Dictionary raw = bridge->send_command_sync(instance_id, "screenshot", params);
        if (raw.has("pending")) return raw;
        Dictionary result = RuntimeBridgeServer::make_response(raw);

        // Add MCP ImageContent type alongside legacy data field
        if (result.has("success") && static_cast<bool>(result["success"])) {
            Dictionary data = result.get("data", Dictionary());
            if (data.has("data") && data.has("mime")) {
                String b64 = data.get("data", "");
                String mime = data.get("mime", "image/png");
                if (!b64.is_empty()) {
                    // MCP ImageContent: type + mimeType + data
                    Dictionary image_content;
                    image_content["type"] = "image";
                    image_content["mimeType"] = mime;
                    image_content["data"] = b64;
                    result["image_content"] = image_content;
                }
            }
        }
        return result;
    }
};

} // namespace godot_mcp
