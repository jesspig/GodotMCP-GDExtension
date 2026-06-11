// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/resource_loader.hpp>

namespace godot_mcp {

class LoadResourceTool : public ITool {
public:
    String name() const override { return "load_resource"; }
    String category() const override { return "node_tools/general"; }
    String brief() const override {
        return String("Load a resource from file");
    }
    String description() const override {
        return String("Loads a resource from a file path and assigns it to a node property. "
                            "Supports .tres, .res, .png, .ogg and all other Godot resource formats.");
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
            p["description"] = String("Resource file path (res:// prefix, e.g. res://assets/icon.png)");
            props["resource_path"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        s["required"] = Array::make("property_name", "resource_path");
        return s;
    }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return false; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String path = args_string(ctx.args, "node_path", "");
        String prop_name = args_string(ctx.args, "property_name");
        String res_path = args_string(ctx.args, "resource_path");

        if (prop_name.is_empty()) {
            return ToolResult::err("MISSING_ARG", String::utf8("property_name 不能为空"));
        }
        if (res_path.is_empty()) {
            return ToolResult::err("MISSING_ARG", String::utf8("resource_path 不能为空"));
        }

        Node *node = resolve_node(ctx.root, path);
        if (!node) {
            return ToolResult::err("NODE_NOT_FOUND",
                String::utf8("节点未找到: ") + path);
        }

        Ref<Resource> res = ResourceLoader::get_singleton()->load(res_path);
        if (res.is_null()) {
            return ToolResult::err("LOAD_FAILED",
                String::utf8("无法加载资源: ") + res_path);
        }

        undoable_set(node, prop_name, res,
            String::utf8("Load ") + res_path + String::utf8(" into ") + prop_name);

        Dictionary data;
        data["node_path"] = relative_path(ctx.root, node);
        data["property_name"] = prop_name;
        data["resource_path"] = res_path;
        data["resource_type"] = res->get_class();
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
