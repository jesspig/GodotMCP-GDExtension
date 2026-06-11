// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/editor_interface.hpp>

namespace godot_mcp {

class RemoveFromGroupTool : public ITool {
public:
    String name() const override { return "remove_from_group"; }
    String category() const override { return "node_tools/group"; }
    String brief() const override {
        return String("Remove a node from a group");
    }
    String description() const override {
        return String("Removes a node from a scene group. Returns an error if the node is not in the group.");
    }
    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String("Node path (empty = root node of current edited scene)");
            props["node_path"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String("Group name");
            props["group_name"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        s["required"] = Array::make("node_path", "group_name");
        return s;
    }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return false; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String path = args_string(ctx.args, "node_path", "");
        String group_name = args_string(ctx.args, "group_name");

        if (group_name.is_empty()) {
            return ToolResult::err("MISSING_ARG", String::utf8("group_name 不能为空"));
        }

        Node *node = resolve_node(ctx.root, path);
        if (!node) {
            return ToolResult::err("NODE_NOT_FOUND",
                String::utf8("节点未找到: ") + path);
        }

        if (!node->is_in_group(group_name)) {
            return ToolResult::err("NOT_IN_GROUP",
                String::utf8("节点不在分组中: ") + group_name);
        }

        node->remove_from_group(group_name);

        Dictionary data;
        data["node"] = relative_path(ctx.root, node);
        data["group"] = group_name;
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
