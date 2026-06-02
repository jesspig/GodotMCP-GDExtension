// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/editor_file_system.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/gd_script.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/script.hpp>

namespace godot_mcp {

class AttachScriptTool : public ITool {
public:
    String name() const override { return "attach_script"; }
    String category() const override { return "node"; }
    String brief() const override { return "Attach a script to a node"; }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return true; }
    String description() const override {
        return "Attaches a GDScript (or any Script) to a node, forcing filesystem sync and @export property registration.";
    }
    Dictionary input_schema() const override {
        Dictionary schema;
        schema["type"] = "object";
        Dictionary props;
        props["node_path"] = []() { Dictionary d; d["type"] = "string"; d["description"] = "Path to the target node"; return d; }();
        props["script_path"] = []() { Dictionary d; d["type"] = "string"; d["description"] = "Path to the .gd script file"; return d; }();
        schema["properties"] = props;
        Array req; req.push_back("node_path"); req.push_back("script_path"); schema["required"] = req;
        return schema;
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        const String sp = args_string(ctx.args, "script_path");
        if (sp.is_empty()) return ToolResult::err("MISSING_PARAM", "missing 'script_path'");

        Node *n = ctx.node;

        EditorFileSystem *fs = EditorInterface::get_singleton()->get_resource_filesystem();
        if (fs) fs->update_file(sp);

        Ref<Script> script = ResourceLoader::get_singleton()->load(sp);
        if (script.is_null()) return ToolResult::err("LOAD_FAILED", "Script '" + sp + "' could not be loaded");

        Ref<GDScript> gdscript = Object::cast_to<GDScript>(script.ptr());
        if (gdscript.is_valid()) {
            const String src = FileAccess::get_file_as_string(sp);
            gdscript->set_source_code(src);
            gdscript->reload();
        }

        Variant old = n->get("script");
        EditorUndoRedoManager *ur = get_undo_redo();
        if (ur) {
            ur->create_action("Attach Script: " + sp);
            ur->add_do_property(n, "script", script);
            ur->add_undo_property(n, "script", old);
            ur->commit_action();
        } else {
            n->set("script", script);
        }
        n->notify_property_list_changed();

        Dictionary data;
        data["node_path"] = relative_path(ctx.root, n);
        data["script_path"] = sp;
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
