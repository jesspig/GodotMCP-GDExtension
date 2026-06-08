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
        return String::utf8("列出节点所属的所有分组");
    }
    String description() const override {
        return String::utf8("返回指定节点所属的所有分组名称列表，以及每个分组的持久化状态。");
    }
    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String::utf8("节点路径（空=当前编辑场景根节点）");
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
