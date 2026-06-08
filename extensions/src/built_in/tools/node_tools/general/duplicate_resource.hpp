// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/resource.hpp>

namespace godot_mcp {

class DuplicateResourceTool : public ITool {
public:
    String name() const override { return "duplicate_resource"; }
    String category() const override { return "node_tools/general"; }
    String brief() const override {
        return String::utf8("复制资源");
    }
    String description() const override {
        return String::utf8("深度复制节点属性上的资源对象，并将副本重新赋给该属性。"
                            "与原资源断开引用关系，修改副本不会影响原资源。");
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

        Variant val = node->get(prop_name);
        Ref<Resource> res = val;
        if (res.is_null()) {
            return ToolResult::err("NOT_A_RESOURCE",
                String::utf8("该属性当前没有 Resource 值"));
        }

        Ref<Resource> dup = res->duplicate(true);
        if (dup.is_null()) {
            return ToolResult::err("DUPLICATE_FAILED",
                String::utf8("资源复制失败"));
        }

        undoable_set(node, prop_name, dup,
            String::utf8("Duplicate ") + prop_name + String::utf8(" on ") + relative_path(ctx.root, node));

        Dictionary data;
        data["node_path"] = relative_path(ctx.root, node);
        data["property_name"] = prop_name;
        data["resource_type"] = dup->get_class();
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
