#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

namespace godot_mcp {

// =====================================================================
// GetNodePropertyTool — Generic fallback: read any property by name
// =====================================================================

class GetNodePropertyTool : public ITool {
public:
    String name() const override { return "get_node_property"; }
    String category() const override { return "node_tools/fallback"; }
    String brief() const override {
        return "Read any property from a node by name";
    }
    String description() const override {
        return "Generic node property reader. Accepts any property name and returns its current value. "
               "Supports all node types and all built-in/custom properties. "
               "Use this when no semantic shortcut tool exists for the property you need.";
    }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return true; }
    Dictionary input_schema() const override {
        Dictionary s;
        s["type"] = "object";
        Dictionary p;
        Dictionary np;
        np["type"] = "string";
        np["description"] = "Node path";
        p["node_path"] = np;
        Dictionary pp;
        pp["type"] = "string";
        pp["description"] = "Property name to read";
        p["property"] = pp;
        s["properties"] = p;
        Array r;
        r.push_back("node_path");
        r.push_back("property");
        s["required"] = r;
        return s;
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String prop = args_string(ctx.args, "property");
        if (prop.is_empty()) {
            return ToolResult::err("MISSING_PARAM", "property is required");
        }
        Variant val = ctx.node->get(prop);
        Dictionary data;
        data["node_path"] = relative_path(ctx.root, ctx.node);
        data["property"] = prop;
        data["value"] = variant_to_json(val);
        return ToolResult::ok(data);
    }
};

// =====================================================================
// SetNodePropertyTool — Generic fallback: write any property by name
// =====================================================================

class SetNodePropertyTool : public ITool {
public:
    String name() const override { return "set_node_property"; }
    String category() const override { return "node_tools/fallback"; }
    String brief() const override {
        return "Write any property on a node by name (with undo)";
    }
    String description() const override {
        return "Generic node property writer. Accepts any property name and value. "
               "Supports all node types and all built-in/custom properties. "
               "Value type conversion is automatic (enums from strings, vectors from objects). "
               "Operations are undoable via the editor undo stack.";
    }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return true; }
    // execute_impl 内部已通过 undoable_set() 自行处理 undo，避免外层空 Undo action
    bool supports_undo() const override { return false; }
    Dictionary input_schema() const override {
        Dictionary s;
        s["type"] = "object";
        Dictionary p;
        Dictionary np;
        np["type"] = "string";
        np["description"] = "Node path";
        p["node_path"] = np;
        Dictionary pp;
        pp["type"] = "string";
        pp["description"] = "Property name to write";
        p["property"] = pp;
        Dictionary vp;
        vp["description"] = "Value to set (supports all Godot types: primitives, strings, vectors, colors, resources, etc.)";
        p["value"] = vp;
        s["properties"] = p;
        Array r;
        r.push_back("node_path");
        r.push_back("property");
        r.push_back("value");
        s["required"] = r;
        return s;
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String prop = args_string(ctx.args, "property");
        if (prop.is_empty()) {
            return ToolResult::err("MISSING_PARAM", "property is required");
        }
        if (!ctx.args.has("value")) {
            return ToolResult::err("MISSING_PARAM", "value is required");
        }
        Variant new_val = json_to_variant(ctx.args["value"]);
        undoable_set(ctx.node, prop, new_val,
                     String("Set ") + prop + String(" for ") + relative_path(ctx.root, ctx.node));
        Dictionary data;
        data["node_path"] = relative_path(ctx.root, ctx.node);
        data["property"] = prop;
        data["value"] = variant_to_json(ctx.node->get(prop));
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
