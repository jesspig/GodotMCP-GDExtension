
#pragma once

#include "built_in/cmd_utils/schema_builder.hpp"
#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "server/registry/handler_registry.hpp"
#include "runtime/bridge.hpp"

namespace godot_mcp {

class GetGameSceneTreeTool : public ITool {
    HandlerRegistry *registry_ = nullptr;
public:
    String name() const noexcept override { return "get_game_scene_tree"; }
    String category() const noexcept override { return "runtime_tools/bridge"; }
    String brief() const noexcept override { return String("Get the scene tree of a running game"); }
    String description() const override {
        return String("Gets the full scene tree of the running game, including name, type and path of all nodes. "
                             "Optional max_depth parameter limits recursion depth (-1 for unlimited).");
    }
    String category_description() const override {
        return "Game runtime bridge tools: scene tree queries, property read/write, script execution, input simulation, screenshot, UI discovery, etc.";
    }
    void set_registry(HandlerRegistry *reg) override { registry_ = reg; }

    Dictionary build_input_schema() const override {
        return SchemaBuilder()
            .prop("max_depth", "integer", "Maximum recursion depth, -1 for unlimited", (int64_t)-1)
            .build();
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

