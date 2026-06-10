// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "server/registry/handler_registry.hpp"
#include "runtime/bridge.hpp"

namespace godot_mcp {

class CaptureGameScreenshotTool : public ITool {
    HandlerRegistry *registry_ = nullptr;
public:
    String name() const override { return "capture_game_screenshot"; }
    String category() const override { return "runtime_tools/bridge"; }
    String brief() const override { return String::utf8("截取运行中游戏的游戏视口"); }
    String description() const override {
        return String::utf8("截取运行中游戏的游戏视口截图，返回 Base64 编码的图片数据。"
                             "format 可选 png（默认）或 jpg。");
    }
    bool is_meta() const override { return false; }
    void set_registry(HandlerRegistry *reg) override { registry_ = reg; }

    Dictionary input_schema() const override {
        Dictionary p;
        {
            Dictionary d;
            d["type"] = "string";
            d["description"] = "图片格式：png 或 jpg（默认 png）";
            d["default"] = "png";
            p["format"] = d;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = p;
        return s;
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        RuntimeBridge *bridge = registry_ ? registry_->get_runtime_bridge() : nullptr;
        if (!bridge || !bridge->is_connected()) {
            return ToolResult::err("GAME_NOT_RUNNING", String::utf8("游戏未运行"));
        }
        Dictionary params;
        params["format"] = args_string(ctx.args, "format", "png");
        return RuntimeBridge::make_response(bridge->send_command("screenshot", params));
    }
};

} // namespace godot_mcp
