// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "server/registry/handler_registry.hpp"
#include "runtime/bridge.hpp"

namespace godot_mcp {

class GetGameSceneTreeTool : public ITool {
    HandlerRegistry *registry_ = nullptr;
public:
    String name() const override { return "get_game_scene_tree"; }
    String category() const override { return "editor_tools/runtime"; }
    String brief() const override { return String::utf8("获取运行中游戏的场景树"); }
    String description() const override {
        return String::utf8("获取运行中游戏的完整场景树，包括所有节点的名称、类型和路径。"
                             "可选参数 max_depth 限制递归深度（默认 -1 为不限）。");
    }
    bool is_meta() const override { return false; }
    void set_registry(HandlerRegistry *reg) override { registry_ = reg; }

    Dictionary input_schema() const override {
        Dictionary p;
        {
            Dictionary d;
            d["type"] = "integer";
            d["description"] = String::utf8("最大递归深度，-1 为不限");
            d["default"] = -1;
            p["max_depth"] = d;
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
        params["max_depth"] = args_int(ctx.args, "max_depth", -1);
        return bridge->send_command("get_scene_tree", params);
    }
};

} // namespace godot_mcp
