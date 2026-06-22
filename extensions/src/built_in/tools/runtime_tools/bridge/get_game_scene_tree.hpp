
#pragma once

#include "built_in/cmd_utils/schema_builder.hpp"
#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "runtime/bridge_server.hpp"
#include "server/registry/handler_registry.hpp"

namespace godot_mcp {

class GetGameSceneTreeTool : public ITool {
    HandlerRegistry *registry_ = nullptr;
public:
    String name() const noexcept override { return "get_game_scene_tree"; }
    String category() const noexcept override { return "runtime_tools/bridge"; }
    String brief() const noexcept override { return String("Get the scene tree of a running game"); }
    String description() const override {
        return String("Gets the full scene tree of the running game, including name, type and path of all nodes. "
                              "Optional max_depth parameter limits recursion depth (-1 for unlimited). "
                              "Returns immediately with a pending token; the actual response arrives via SSE.");
    }
    String category_description() const override {
        return "Game runtime bridge tools: scene tree queries, property read/write, script execution, input simulation, screenshot, UI discovery, etc.";
    }
    void set_registry(HandlerRegistry *reg) override { registry_ = reg; }

    Dictionary build_input_schema() const override {
        return SchemaBuilder()
            .prop("max_depth", "integer", "Maximum recursion depth, -1 for unlimited", (int64_t)-1)
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
        params["max_depth"] = args_int(ctx.args, "max_depth", -1);
        Dictionary raw = bridge->send_command_sync(instance_id, "get_scene_tree", params);
        if (raw.has("pending")) return raw;
        return RuntimeBridgeServer::make_response(raw);
    }
};

} // namespace godot_mcp
