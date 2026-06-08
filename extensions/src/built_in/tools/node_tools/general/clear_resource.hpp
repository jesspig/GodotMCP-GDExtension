// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/editor_interface.hpp>

namespace godot_mcp {

class ClearResourceTool : public ITool {
public:
    String name() const override { return "clear_resource"; }
    String category() const override { return "node_tools/general"; }
    String brief() const override {
        return String::utf8("清除资源引用");
    }
    String description() const override {
        return String::utf8("将节点属性上的资源引用置空。如果属性需要资源才能正常工作，"
                            "后续可调用 new_resource 创建新的默认实例。");
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
            p["description"] = String::utf8("属性名称（如 texture、material）");
            props["property_name"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        s["required"] = Array::make("property_name");
        return s;
    }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return false; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String path = args_string(ctx.args, "node_path", "");
        String prop_name = args_string(ctx.args, "property_name");

        if (prop_name.is_empty()) {
            return ToolResult::err("MISSING_ARG", String::utf8("property_name 不能为空"));
        }

        Node *node = resolve_node(ctx.root, path);
        if (!node) {
            return ToolResult::err("NODE_NOT_FOUND",
                String::utf8("节点未找到: ") + path);
        }

        undoable_set(node, prop_name, Variant(),
            String::utf8("Clear ") + prop_name + String::utf8(" on ") + relative_path(ctx.root, node));

        Dictionary data;
        data["node_path"] = relative_path(ctx.root, node);
        data["property_name"] = prop_name;
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
