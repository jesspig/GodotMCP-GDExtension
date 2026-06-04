// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/resource.hpp>

namespace godot_mcp {

class NewResourceTool : public ITool {
public:
    String name() const override { return "new_resource"; }
    String category() const override { return "node_tools/general"; }
    String brief() const override {
        return String::utf8("创建新的默认资源");
    }
    String description() const override {
        return String::utf8("为节点属性创建一个新的默认资源实例。"
                            "如果属性已有资源，将被替换为新实例。"
                            "资源类型由属性定义决定，也可以通过 resource_type 参数指定。");
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
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String::utf8("可选的资源类型名（如 Texture2D、Material），不填则从属性定义自动检测");
            props["resource_type"] = p;
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
        String res_type = args_string(ctx.args, "resource_type", "");

        if (prop_name.is_empty()) {
            return ToolResult::err("MISSING_ARG", String::utf8("property_name 不能为空"));
        }

        Node *node = resolve_node(ctx.root, path);
        if (!node) {
            return ToolResult::err("NODE_NOT_FOUND",
                String::utf8("节点未找到: ") + path);
        }

        // 如果未指定资源类型，从属性定义自动检测
        if (res_type.is_empty()) {
            Array prop_list = node->get_property_list();
            for (int i = 0; i < prop_list.size(); i++) {
                Dictionary p = prop_list[i];
                String pname = p.get("name", "");
                if (pname == prop_name) {
                    res_type = p.get("class_name", "");
                    break;
                }
            }
        }

        if (res_type.is_empty()) {
            return ToolResult::err("UNKNOWN_TYPE",
                String::utf8("无法确定资源类型，请通过 resource_type 参数指定"));
        }

        Object *new_obj = ClassDB::instantiate(res_type);
        if (!new_obj) {
            return ToolResult::err("INSTANTIATE_FAILED",
                String::utf8("无法实例化资源类型: ") + res_type);
        }

        Resource *new_res = Object::cast_to<Resource>(new_obj);
        if (!new_res) {
            memdelete(new_obj);
            return ToolResult::err("NOT_A_RESOURCE",
                String::utf8("指定类型不是 Resource: ") + res_type);
        }

        Ref<Resource> ref(new_res);

        undoable_set(node, prop_name, ref,
            String::utf8("New ") + res_type + String(" for ") + prop_name
            + String::utf8(" on ") + relative_path(ctx.root, node));

        Dictionary data;
        data["node_path"] = relative_path(ctx.root, node);
        data["property_name"] = prop_name;
        data["resource_type"] = res_type;
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
