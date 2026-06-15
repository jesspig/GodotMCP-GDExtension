#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/variant/node_path.hpp>

namespace godot_mcp {

class GetNodesInGroupTool : public ITool {
public:
    String name() const override { return "get_nodes_in_group"; }
    String category() const override { return "node_tools/group"; }
    String brief() const override {
        return String("Get all nodes in a group");
    }
    String description() const override {
        return String("Returns the name, type and path of all nodes belonging to the specified group in the current scene tree.");
    }
    Dictionary build_input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String("Group name");
            props["group_name"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        s["required"] = Array::make("group_name");
        return s;
    }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return false; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String group_name = args_string(ctx.args, "group_name");
        if (group_name.is_empty()) {
            return ToolResult::err("MISSING_ARG", String("group_name cannot be empty"));
        }

        godot::SceneTree *tree = ctx.root->get_tree();
        if (!tree) {
            return ToolResult::err("NO_TREE", String("Cannot get scene tree"));
        }

        godot::TypedArray<Node> nodes = tree->get_nodes_in_group(group_name);
        Array result;
        for (int i = 0; i < nodes.size(); i++) {
            Node *n = Object::cast_to<Node>(nodes[i]);
            if (!n) continue;
            Dictionary entry;
            entry["name"] = n->get_name();
            entry["type"] = n->get_class();
            entry["path"] = relative_path(ctx.root, n);
            result.push_back(entry);
        }

        Dictionary data;
        data["group"] = group_name;
        data["nodes"] = result;
        data["count"] = static_cast<int64_t>(result.size());
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
