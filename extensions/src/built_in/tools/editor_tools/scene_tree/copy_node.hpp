
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "scene_tree_utils.hpp"

namespace godot_mcp {

class CopyNodeTool : public ITool {
public:
    String name() const override { return "copy_node"; }
    String category() const override { return "editor_tools/scene_tree"; }
    String brief() const override {
        return "Copy a node to the internal clipboard";
    }
    String description() const override {
        return "Serializes the node subtree into an internal PackedScene clipboard, for use by paste_node. "
               "The clipboard persists within the Godot process across MCP tool calls. "
               "Carries node and scene instance data (with editor state), separate from the Godot editor's internal clipboard.";
    }
    Dictionary build_input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Node path (empty = scene root, not allowed)";
            props["node_path"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        s["required"] = Array::make("node_path");
        return s;
    }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return false; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String node_path = args_string(ctx.args, "node_path");
        Node *node = resolve_node(ctx.root, node_path);
        if (!node) {
            return ToolResult::err("NODE_NOT_FOUND",
                "Node not found: " + node_path);
        }
        if (node == ctx.root) {
            return ToolResult::err("ROOT_NOT_ALLOWED",
                "Cannot copy the scene root node");
        }

        godot::Ref<godot::PackedScene> scene = scene_tree_utils::pack_subtree(node);
        if (scene.is_null()) {
            return ToolResult::err("PACK_FAILED",
                "Failed to pack node");
        }
        scene_tree_utils::set_clipboard(scene);

        Dictionary data;
        data["copied"] = relative_path(ctx.root, node);
        data["type"] = node->get_class();
        data["node_count"] = static_cast<int64_t>(node->get_child_count()) + 1;
        return ToolResult::ok(data);
    }
};

}  // namespace godot_mcp
