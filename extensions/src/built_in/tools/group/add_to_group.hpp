#pragma once

#include "built_in/cmd_utils/schema_builder.hpp"
#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "built_in/tools/editor_tools/scene_tree/scene_tree_utils.hpp"

#include <godot_cpp/classes/editor_interface.hpp>

namespace godot_mcp {

class AddToGroupTool : public ITool {
public:
    String name() const noexcept override { return "add_to_group"; }
    String category() const noexcept override { return "node_tools/group"; }
    String brief() const noexcept override {
        return String("Add a node to a group");
    }
    String description() const override {
        return String("Adds a node to a scene group. "
                            "persistent=true means the group relationship is saved when the scene is saved.");
    }
    Dictionary build_input_schema() const override {
        return SchemaBuilder()
            .prop("node_path", "string", "Node path (empty = root node of current edited scene)")
            .prop("group_name", "string", "Group name")
            .prop("persistent", "boolean", "Whether to persist (save to scene file)")
            .required({"node_path", "group_name"})
            .build();
    }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return false; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String path = args_string(ctx.args, "node_path", "");
        String group_name = args_string(ctx.args, "group_name");
        bool persistent = args_bool(ctx.args, "persistent", false);

        if (group_name.is_empty()) {
            return ToolResult::err("MISSING_ARG", String("group_name cannot be empty"));
        }

        Node *node = resolve_node(ctx.root, path);
        if (!node) {
            return ToolResult::err("NODE_NOT_FOUND",
                String("Node not found: ") + path);
        }

        if (node->is_in_group(group_name)) {
            return ToolResult::err("ALREADY_IN_GROUP",
                String("Node already in group: ") + group_name);
        }

        auto *ur = get_undo_redo();
        if (ur) {
            ur->create_action("MCP: Add to group", godot::UndoRedo::MERGE_DISABLE, ctx.root);
            ur->add_do_method(node, "add_to_group", group_name, persistent);
            ur->add_undo_method(node, "remove_from_group", group_name);
            ur->commit_action();
        } else {
            node->add_to_group(group_name, persistent);
        }

        Dictionary data;
        data["node"] = relative_path(ctx.root, node);
        data["group"] = group_name;
        data["persistent"] = persistent;
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
