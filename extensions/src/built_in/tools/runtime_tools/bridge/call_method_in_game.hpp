
#pragma once

#include "built_in/cmd_utils/schema_builder.hpp"
#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "runtime/bridge_server.hpp"
#include "server/registry/handler_registry.hpp"

namespace godot_mcp {

class CallMethodInGameTool : public ITool {
    HandlerRegistry *registry_ = nullptr;
public:
    String name() const noexcept override { return "call_method_in_game"; }
    String category() const noexcept override { return "runtime_tools/bridge"; }
    String brief() const noexcept override { return String("Call a method on a running game node"); }
    String description() const override {
        return String("Calls an arbitrary method on a node in the running game and returns the result. "
                      "node_path is the node path (e.g. /root/Main/Player), "
                      "method is the method name, args is the method argument array. "
                      "Args support all Godot types: primitives, strings, vectors (as {x,y} objects), "
                      "colors (as {r,g,b,a} objects), and nested arrays/dictionaries. "
                      "Returns the method's return value with its type name. "
                      "Returns immediately with a pending token; the actual response arrives via SSE.");
    }
    void set_registry(HandlerRegistry *reg) override { registry_ = reg; }

    Dictionary build_input_schema() const override {
        return SchemaBuilder()
            .prop("node_path", "string", "Node path, e.g. /root/Main/Player")
            .prop("method", "string", "Method name")
            .prop("args", "array", "Method argument array (optional, supports all Godot types)")
            .prop("instance_id", "integer", "Game instance ID (default: first connected instance)", (int64_t)0)
            .required({"node_path", "method"})
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
        params["node_path"] = args_string(ctx.args, "node_path");
        params["method"] = args_string(ctx.args, "method");
        params["args"] = ctx.args.get("args", Array());
        Dictionary raw = bridge->send_command_sync(instance_id, "call_method", params);
        if (raw.has("pending")) return raw;
        Dictionary result = RuntimeBridgeServer::make_response(raw);

        // Convert return value to JSON-friendly format
        if (result.has("success") && static_cast<bool>(result["success"])) {
            Dictionary data = result.get("data", Dictionary());
            if (data.has("data")) {
                Variant ret = data["data"];
                data["data"] = variant_to_json(ret);
                result["data"] = data;
            }
        }
        return result;
    }
};

} // namespace godot_mcp
