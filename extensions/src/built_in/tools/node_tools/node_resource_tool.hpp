// node_resource_tool.hpp �?通用资源属�?Get/Set 工具模板
//
// 两个通用 ITool 子类，通过 codegen �?YAML 属性数据库动态注册�?
// 用于读取/设置资源对象上的属性，而非节点上的属性�?
// 资源通过 node_path + property_name 定位�?

#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector2i.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/variant/vector3i.hpp>
#include <godot_cpp/variant/vector4.hpp>
#include <godot_cpp/variant/quaternion.hpp>

namespace godot_mcp {

// =====================================================================
// NodeResourceGetTool �?读取节点上资源对象的属�?
// =====================================================================

class NodeResourceGetTool : public ITool {
    String name_;
    String category_;
    String res_type_;
    String prop_name_;
    String brief_;
    String cat_desc_;
    Dictionary schema_;

public:
    NodeResourceGetTool(const String &name, const String &category,
                        const String &res_type, const String &prop_name,
                        const String &cat_desc = {}) :
        name_(name), category_(category),
        res_type_(res_type), prop_name_(prop_name), cat_desc_(cat_desc) {
        brief_ = String("Get ") + res_type_ + String(".") + prop_name_;
        Dictionary s;
        s["type"] = "object";
        Dictionary p;
        Dictionary np;
        np["type"] = "string";
        np["description"] = String("Node path");
        p["node_path"] = np;
        Dictionary pp;
        pp["type"] = "string";
        pp["description"] = String("Node property name that holds the resource");
        p["property_name"] = pp;
        s["properties"] = p;
        Array r;
        r.push_back("node_path");
        r.push_back("property_name");
        s["required"] = r;
        schema_ = s;
    }

    String name() const override { return name_; }
    String category() const override { return category_; }
    String brief() const override { return brief_; }
    String description() const override {
        return String("Reads the ") + prop_name_
            + String(" property of a ") + res_type_
            + String(" resource on a node.");
    }
    String category_description() const override { return cat_desc_; }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return false; }
    Dictionary input_schema() const override { return schema_; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String prop_name = args_string(ctx.args, "property_name");
        Node *node = resolve_node(ctx.root, args_string(ctx.args, "node_path", ""));
        if (!node) {
            return ToolResult::err("NODE_NOT_FOUND",
                String("Node not found"));
        }

        Variant val = node->get(prop_name);
        Ref<Resource> res = val;
        if (res.is_null()) {
            return ToolResult::err("NOT_A_RESOURCE",
                String("The property does not have a Resource value"));
        }

        Variant prop_val = res->get(prop_name_);
        Dictionary data;
        data["node_path"] = relative_path(ctx.root, node);
        data["property_name"] = prop_name;
        data["resource_type"] = res_type_;
        data[prop_name_] = variant_to_json(prop_val);
        return ToolResult::ok(data);
    }
};

// =====================================================================
// NodeResourceSetTool �?设置节点上资源对象的属性（带撤销支持�?
// =====================================================================

class NodeResourceSetTool : public ITool {
    String name_;
    String category_;
    String res_type_;
    String prop_name_;
    String brief_;
    String cat_desc_;
    Dictionary schema_;

public:
    NodeResourceSetTool(const String &name, const String &category,
                        const String &res_type, const String &prop_name,
                        const String &cat_desc = {}) :
        name_(name), category_(category),
        res_type_(res_type), prop_name_(prop_name), cat_desc_(cat_desc) {
        brief_ = String("Set ") + res_type_ + String(".") + prop_name_;
        Dictionary s;
        s["type"] = "object";
        Dictionary p;
        Dictionary np;
        np["type"] = "string";
        np["description"] = String("Node path");
        p["node_path"] = np;
        Dictionary pp;
        pp["type"] = "string";
        pp["description"] = String("Node property name that holds the resource");
        p["property_name"] = pp;
        Dictionary vp;
        vp["type"] = "object";
        vp["description"] = String("New value for ") + res_type_ + String(".") + prop_name_
            + String(" (complex types use object format, e.g. {x,y} for Vector2)");
        p["value"] = vp;
        s["properties"] = p;
        Array r;
        r.push_back("node_path");
        r.push_back("property_name");
        r.push_back("value");
        s["required"] = r;
        schema_ = s;
    }

    String name() const override { return name_; }
    String category() const override { return category_; }
    String brief() const override { return brief_; }
    String description() const override {
        return String("Sets the ") + prop_name_
            + String(" property of a ") + res_type_
            + String(" resource with undo support.");
    }
    String category_description() const override { return cat_desc_; }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return false; }
    Dictionary input_schema() const override { return schema_; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String prop_name = args_string(ctx.args, "property_name");
        Node *node = resolve_node(ctx.root, args_string(ctx.args, "node_path", ""));
        if (!node) {
            return ToolResult::err("NODE_NOT_FOUND",
                String("Node not found"));
        }

        if (!ctx.args.has("value")) {
            return ToolResult::err("MISSING_PARAM", "Missing required parameter: value");
        }

        Variant val = node->get(prop_name);
        Ref<Resource> res = val;
        if (res.is_null()) {
            return ToolResult::err("NOT_A_RESOURCE",
                String("The property does not have a Resource value"));
        }

        Variant new_val = json_to_variant(ctx.args["value"]);
        res->set(prop_name_, new_val);

        Dictionary data;
        data["node_path"] = relative_path(ctx.root, node);
        data["property_name"] = prop_name;
        data["resource_type"] = res_type_;
        data[prop_name_] = variant_to_json(res->get(prop_name_));
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
