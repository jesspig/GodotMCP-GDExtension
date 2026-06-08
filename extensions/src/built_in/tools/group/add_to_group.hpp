// @tool register
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
        return String::utf8("将节点添加到指定分组");
    }
    String description() const override {
        return String::utf8("将一个节点添加到场景分组（Group）中。"
                            "persistent=true 表示该分组关系在场景保存时持久化。");
    }
    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String::utf8("节点路径（空=当前编辑场景根节点）");
            props["node_path"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String::utf8("分组名称");
            props["group_name"] = p;
        }
        {
            Dictionary p;
            p["type"] = "boolean";
            p["description"] = String::utf8("是否持久化（保存到场景文件）");
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
                String::utf8("节点未找到: ") + path);
        }

        if (node->is_in_group(group_name)) {
            return ToolResult::err("ALREADY_IN_GROUP",
                String::utf8("节点已在分组中: ") + group_name);
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
