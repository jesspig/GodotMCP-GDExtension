
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
    String category() const override { return "runtime_tools/bridge"; }
    String brief() const override { return String("Get the scene tree of a running game"); }
    String description() const override {
        return String("Gets the full scene tree of the running game, including name, type and path of all nodes. "
                             "Optional max_depth parameter limits recursion depth (-1 for unlimited).");
    }
    bool is_meta() const override { return false; }
    void set_registry(HandlerRegistry *reg) override { registry_ = reg; }

    Dictionary input_schema() const override {
        Dictionary p;
        {
            Dictionary d;
            d["type"] = "integer";
            d["description"] = String("Maximum recursion depth, -1 for unlimited");
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
            return ToolResult::err("GAME_NOT_RUNNING", "Game not running");
        }
        Dictionary params;
        params["max_depth"] = args_int(ctx.args, "max_depth", -1);
        return RuntimeBridge::make_response(bridge->send_command("get_scene_tree", params));
    }
};

} // namespace godot_mcp

