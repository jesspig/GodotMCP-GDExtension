// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "server/registry/handler_registry.hpp"
#include "runtime/bridge.hpp"

namespace godot_mcp {

class PauseProjectTool : public ITool {
    HandlerRegistry *registry_ = nullptr;
public:
    String name() const override { return "pause_project"; }
    String category() const override { return "runtime_tools/lifecycle"; }
    String brief() const override { return String::utf8("暂停或恢复运行中的项目"); }
    String description() const override {
        return String::utf8("暂停或恢复运行中的项目。暂停时游戏场景的 _process 和 _physics_process 停止处理，"
                             "但引擎继续运行。等同于调用 SceneTree.set_pause()。"
                             "此功能需要游戏通过运行时桥接连接（:9601）。"
                             "如果游戏未运行或桥接未连接则返回错误。");
    }
    void set_registry(HandlerRegistry *reg) override { registry_ = reg; }

    Dictionary input_schema() const override {
        Dictionary p;
        {
            Dictionary d;
            d["type"] = "boolean";
            d["description"] = String::utf8("true 暂停，false 恢复");
            p["paused"] = d;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = p;
        s["required"] = Array::make("paused");
        return s;
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        RuntimeBridge *bridge = registry_ ? registry_->get_runtime_bridge() : nullptr;
        if (!bridge || !bridge->is_connected()) {
            return ToolResult::err("GAME_NOT_RUNNING", String::utf8("游戏未运行或桥接未连接"));
        }
        bool paused = args_bool(ctx.args, "paused");
        Dictionary params;
        params["paused"] = paused;
        return bridge->send_command("set_pause", params);
    }
};

} // namespace godot_mcp
