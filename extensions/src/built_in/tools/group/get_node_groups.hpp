
#pragma once

#include "built_in/cmd_utils/schema_builder.hpp"
#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "built_in/tools/editor_tools/scene_tree/scene_tree_utils.hpp"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>

namespace godot_mcp {

class GetNodeGroupsTool : public ITool {
public:
    String name() const noexcept override { return "get_node_groups"; }
    String category() const noexcept override { return "node_tools/group"; }
    String brief() const noexcept override {
        return String("List all groups a node belongs to");
    }
    String description() const override {
        return String("Returns the list of all group names the specified node belongs to, along with the persistence status of each group.");
    }
    Dictionary build_input_schema() const override {
        return SchemaBuilder()
            .prop("node_path", "string", "Node path (empty = root node of current edited scene)")
            .build();
    }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return false; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String path = args_string(ctx.args, "node_path", "");
        Node *node = nullptr;
        if (auto err = scene_tree_utils::resolve_node_or_error(ctx.root, path, node)) {
            return ToolResult::err("NODE_NOT_FOUND", err->get("message", ""));
        }

        PackedStringArray groups = node->get_groups();
        Array result;
        for (int i = 0; i < groups.size(); i++) {
            result.push_back(groups[i]);
        }

        Dictionary data;
        data["node"] = relative_path(ctx.root, node);
        data["groups"] = result;
        data["count"] = static_cast<int64_t>(result.size());
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp

