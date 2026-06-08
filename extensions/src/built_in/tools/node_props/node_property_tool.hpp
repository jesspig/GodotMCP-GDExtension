// node_property_tool.hpp — 通用节点属性 Get/Set 工具模板
//
// 两个通用 ITool 子类，通过 codegen 从 YAML 属性数据库动态注册。
// 不需要每个属性一个 .hpp 文件——只需构造函数参数即可。

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
// NodePropertyGetTool — 获取节点的单个属性
// =====================================================================

class NodePropertyGetTool : public ITool {
    String name_;
    String category_;
    String node_type_;
    String prop_name_;
    String brief_;
    String cat_desc_;
    Dictionary schema_;

public:
    NodePropertyGetTool(const String &name, const String &category,
                        const String &node_type, const String &prop_name,
                        const String &cat_desc = {}) :
        name_(name), category_(category),
        node_type_(node_type), prop_name_(prop_name), cat_desc_(cat_desc) {
        brief_ = String("Get ") + node_type_ + String(".") + prop_name_;
        Dictionary s;
        s["type"] = "object";
        Dictionary p;
        Dictionary np;
        np["type"] = "string";
        np["description"] = "Node path";
        p["node_path"] = np;
        s["properties"] = p;
        Array r;
        r.push_back("node_path");
        s["required"] = r;
        schema_ = s;
    }

    String name() const override { return name_; }
    String category() const override { return category_; }
    String brief() const override { return brief_; }
    String description() const override {
        return String("Gets the ") + prop_name_ + String(" property of a ") + node_type_ + String(".");
    }
    String category_description() const override { return cat_desc_; }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return true; }
    Dictionary input_schema() const override { return schema_; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        if (!ctx.node->is_class(node_type_)) {
            return ToolResult::err("WRONG_NODE_TYPE",
                String("Expected ") + node_type_ + String(" or derived, got ") + ctx.node->get_class());
        }
        Variant val = ctx.node->get(prop_name_);
        Dictionary data;
        data["node_path"] = relative_path(ctx.root, ctx.node);
        data[prop_name_] = variant_to_json(val);
        return ToolResult::ok(data);
    }
};

// =====================================================================
// NodePropertySetTool — 设置节点的单个属性（带撤销支持）
// =====================================================================

class NodePropertySetTool : public ITool {
    String name_;
    String category_;
    String node_type_;
    String prop_name_;
    String brief_;
    String cat_desc_;
    Dictionary schema_;

public:
    NodePropertySetTool(const String &name, const String &category,
                        const String &node_type, const String &prop_name,
                        const String &cat_desc = {}) :
        name_(name), category_(category),
        node_type_(node_type), prop_name_(prop_name), cat_desc_(cat_desc) {
        brief_ = String("Set ") + node_type_ + String(".") + prop_name_;
        Dictionary s;
        s["type"] = "object";
        Dictionary p;
        Dictionary np;
        np["type"] = "string";
        np["description"] = "Node path";
        p["node_path"] = np;
        Dictionary vp;
        vp["type"] = "object";
        vp["description"] = String("Value for ") + prop_name_ +
            String(" (complex types use object format, e.g. {x,y} for Vector2)");
        p["value"] = vp;
        s["properties"] = p;
        Array r;
        r.push_back("node_path");
        r.push_back("value");
        s["required"] = r;
        schema_ = s;
    }

    String name() const override { return name_; }
    String category() const override { return category_; }
    String brief() const override { return brief_; }
    String description() const override {
        return String("Sets the ") + prop_name_ + String(" property of a ") + node_type_ +
            String(" with undo support.");
    }
    String category_description() const override { return cat_desc_; }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return true; }
    Dictionary input_schema() const override { return schema_; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        if (!ctx.node->is_class(node_type_)) {
            return ToolResult::err("WRONG_NODE_TYPE",
                String("Expected ") + node_type_ + String(" or derived, got ") + ctx.node->get_class());
        }
        if (!ctx.args.has("value")) {
            return ToolResult::err("MISSING_PARAM", "Missing required parameter: value");
        }
        Variant new_val = json_to_variant(ctx.args["value"]);
        undoable_set(ctx.node, prop_name_, new_val,
                     String("Set ") + prop_name_ + String(" for ") + relative_path(ctx.root, ctx.node));
        Dictionary data;
        data["node_path"] = relative_path(ctx.root, ctx.node);
        data["property"] = prop_name_;
        data[prop_name_] = variant_to_json(ctx.node->get(prop_name_));
        return ToolResult::ok(data);
    }
};

}  // namespace godot_mcp
