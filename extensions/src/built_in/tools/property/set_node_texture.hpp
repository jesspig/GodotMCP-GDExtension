// @tool register
// @source builtin
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/texture2d.hpp>

namespace godot_mcp {

class SetNodeTextureTool : public ITool {
public:
    String name() const override { return "set_node_texture"; }
    String category() const override { return "property"; }
    String brief() const override { return "Set a texture on a node"; }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return true; }
    String description() const override { return "Sets a texture property on a node (e.g., Sprite2D.texture) from a path."; }
    Dictionary input_schema() const override {
        Dictionary s; s["type"] = "object"; Dictionary p;
        p["node_path"] = [](){Dictionary d;d["type"]="string";d["description"]="Node path";return d;}();
        p["property"] = [](){Dictionary d;d["type"]="string";d["description"]="Texture property name (default: texture)";return d;}();
        p["texture_path"] = [](){Dictionary d;d["type"]="string";d["description"]="Path to the texture resource";return d;}();
        s["properties"] = p; Array r; r.push_back("node_path"); r.push_back("texture_path"); s["required"] = r; return s;
    }
protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String prop = args_string(ctx.args, "property", "texture");
        String tex_path = args_string(ctx.args, "texture_path");
        if (tex_path.is_empty()) return ToolResult::err("MISSING_PARAM", "missing 'texture_path'");
        String p = relative_path(ctx.root, ctx.node);
        Ref<Texture2D> tex = ResourceLoader::get_singleton()->load(tex_path);
        if (tex.is_null()) return ToolResult::err("LOAD_FAILED", "Failed to load Texture2D from '" + tex_path + "'");
        Variant old = ctx.node->get(prop);
        ctx.node->set(prop, tex);
        EditorUndoRedoManager *ur = get_undo_redo();
        if (ur) { ur->create_action("Set " + prop + " for " + p); ur->add_do_property(ctx.node, prop, tex); ur->add_undo_property(ctx.node, prop, old); ur->commit_action(false); }
        Dictionary d; d["node_path"] = p; d["property"] = prop; d["texture_path"] = tex_path;
        return ToolResult::ok(d);
    }
};

} // namespace
