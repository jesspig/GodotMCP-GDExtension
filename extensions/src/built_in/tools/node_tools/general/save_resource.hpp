// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/resource_saver.hpp>

namespace godot_mcp {

class SaveResourceTool : public ITool {
public:
    String name() const override { return "save_resource"; }
    String category() const override { return "node_tools/general"; }
    String brief() const override {
        return String("Save a resource to file");
    }
    String description() const override {
        return String("Saves the resource on a node property to a file path. Supports .tres (text) and .res (binary) formats.");
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
            p["description"] = String("Property name (e.g. texture, material)");
            props["property_name"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String("Save path (res:// or user:// prefix, e.g. res://assets/my_resource.tres)");
            props["save_path"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        s["required"] = Array::make("property_name", "save_path");
        return s;
    }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return false; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String path = args_string(ctx.args, "node_path", "");
        String prop_name = args_string(ctx.args, "property_name");
        String save_path = args_string(ctx.args, "save_path");

        if (prop_name.is_empty()) {
            return ToolResult::err("MISSING_ARG", String::utf8("property_name 不能为空"));
        }
        if (save_path.is_empty()) {
            return ToolResult::err("MISSING_ARG", String::utf8("save_path 不能为空"));
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

        ensure_parent_dir(save_path);
        Error err = ResourceSaver::get_singleton()->save(res, save_path);
        if (err != OK) {
            return ToolResult::err("SAVE_FAILED",
                String::utf8("保存失败，错误码: ") + itos(err));
        }

        notify_file_changed(save_path);

        Dictionary data;
        data["node_path"] = relative_path(ctx.root, node);
        data["property_name"] = prop_name;
        data["save_path"] = save_path;
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
