// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>

namespace godot_mcp {

class GetNodeGroupsTool : public ITool {
public:
    String name() const override { return "get_node_groups"; }
    String category() const override { return "node_tools/group"; }
    String brief() const override {
        return String("List all groups a node belongs to");
    }
    String description() const override {
        return String("Returns the list of all group names the specified node belongs to, along with the persistence status of each group.");
    }
    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String("Node path (empty = root node of current edited scene)");
            props["node_path"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        s["required"] = Array::make();
        return s;
    }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return false; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String path = args_string(ctx.args, "node_path", "");
        Node *node = resolve_node(ctx.root, path);
        if (!node) {
            return ToolResult::err("NODE_NOT_FOUND",
                String::utf8("节点未找到: ") + path);
        }

        PackedStringArray groups = node->get_groups();
        Array result;
        for (int i = 0; i < groups.size(); i++) {
            result.push_back(groups[i]);
        }

        Dictionary data;
        data["node"] = relative_path(ctx.root, node);
        data["groups"] = result;
        data["count"] = (int64_t)result.size();
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
