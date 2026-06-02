// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/texture2d.hpp>

namespace godot_mcp {

class GetNodeTextureTool : public ITool {
public:
    String name() const override { return "get_node_texture"; }
    String category() const override { return "property"; }
    String brief() const override { return "Get the texture path from a node"; }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return true; }
    String description() const override { return "Gets the texture path from a texture property on a node."; }
    Dictionary input_schema() const override {
        Dictionary s; s["type"] = "object"; Dictionary p;
        p["node_path"] = [](){Dictionary d;d["type"]="string";d["description"]="Node path";return d;}();
        p["property"] = [](){Dictionary d;d["type"]="string";d["description"]="Texture property name (default: texture)";return d;}();
        s["properties"] = p; Array r; r.push_back("node_path"); s["required"] = r; return s;
    }
protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String prop = args_string(ctx.args, "property", "texture");
        String p = relative_path(ctx.root, ctx.node);
        Variant val = ctx.node->get(prop);
        if (val.get_type() == Variant::NIL) return ToolResult::err("NO_PROPERTY", "Node '" + p + "' does not have property '" + prop + "'");
        Ref<Texture2D> tex = val;
        Dictionary d; d["node_path"] = p; d["property"] = prop;
        d["texture_path"] = tex.is_valid() ? Variant(tex->get_path()) : Variant();
        return ToolResult::ok(d);
    }
};

} // namespace
