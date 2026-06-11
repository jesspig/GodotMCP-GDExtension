#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/editor_interface.hpp>

namespace godot_mcp {

class AddToGroupTool : public ITool {
public:
    String name() const override { return "add_to_group"; }
    String category() const override { return "node_tools/group"; }
    String brief() const override {
        return String("Add a node to a group");
    }
    String description() const override {
        return String("Adds a node to a scene group. "
                            "persistent=true means the group relationship is saved when the scene is saved.");
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
        {
            Dictionary p;
            p["type"] = "boolean";
            p["description"] = String("Whether to persist (save to scene file)");
            props["persistent"] = p;
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
        bool persistent = args_bool(ctx.args, "persistent", false);

        if (group_name.is_empty()) {
            return ToolResult::err("MISSING_ARG", String::utf8("group_name 不能为空"));
        }

        Node *node = resolve_node(ctx.root, path);
        if (!node) {
            return ToolResult::err("NODE_NOT_FOUND",
                String::utf8("节点未找�? ") + path);
        }

        if (node->is_in_group(group_name)) {
            return ToolResult::err("ALREADY_IN_GROUP",
                String::utf8("节点已在分组�? ") + group_name);
        }

        node->add_to_group(group_name, persistent);

        Dictionary data;
        data["node"] = relative_path(ctx.root, node);
        data["group"] = group_name;
        data["persistent"] = persistent;
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
