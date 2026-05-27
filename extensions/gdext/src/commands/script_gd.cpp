// commands/script_gd.cpp — GDScript CRUD + validate

#include "cmd_utils.hpp"
#include "handler_registry.hpp"
#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_settings.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/gd_script.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/resource_saver.hpp>
#include "../lsp/client.hpp"
#include <godot_cpp/variant/packed_string_array.hpp>
using namespace godot;

namespace godot_mcp {
namespace {

Dictionary cmd_create_gdscript(const Dictionary &a) {
    String path = args_string(a, "path");
    if (path.is_empty()) return make_error("missing 'path'");
    if (!path.ends_with(".gd")) return make_error("path must end with .gd");
    bool overwrite = args_bool(a, "overwrite", false);
    if (FileAccess::file_exists(path) && !overwrite) return make_error("File exists; set overwrite=true");
    String base = args_string(a, "base_class", "RefCounted");
    String cn = args_string(a, "class_name");
    String template_str = args_string(a, "template", args_string(a, "content"));
    bool empty_body = (template_str == "empty");
    String body;
    if (empty_body) {
        body = "extends " + base + "\n" + (cn.is_empty() ? "" : "class_name " + cn + "\n");
    } else if (!template_str.is_empty() && template_str != "empty") {
        body = template_str;
        body = body.replace("_BASE_", base).replace("_CLASS_", cn);
    } else {
        body = "extends " + base + "\n" + (cn.is_empty() ? "" : "class_name " + cn + "\n") +
            "# Called when the node enters the scene tree.\nfunc _ready() -> void:\n\tpass\n\nfunc _process(delta: float) -> void:\n\tpass\n";
    }
    ensure_parent_dir(path);
    Ref<FileAccess> f = FileAccess::open(path, FileAccess::WRITE);
    if (f.is_null()) return make_error("Cannot open file for writing: " + path);
    f->store_string(body);
    notify_file_changed(path);
    Dictionary r; r["path"] = path; r["created"] = true; r["bytes"] = (int64_t)body.length(); r["language"] = "gdscript"; return r;
}
Dictionary cmd_read_gdscript(const Dictionary &a) {
    String path = args_string(a, "path");
    if (!FileAccess::file_exists(path)) return make_error("File not found: " + path);
    String content = FileAccess::get_file_as_string(path);
    Dictionary r; r["path"] = path; r["source"] = content; r["length"] = (int64_t)content.length(); r["language"] = "gdscript"; return r;
}
Dictionary cmd_edit_gdscript(const Dictionary &a) {
    String path = args_string(a, "path"), source = args_string(a, "source");
    if (!path.ends_with(".gd")) return make_error("path must end with .gd");
    if (!FileAccess::file_exists(path)) return make_error("File not found: " + path);
    if (source.is_empty()) return make_error("missing 'source'");
    Ref<FileAccess> f = FileAccess::open(path, FileAccess::WRITE);
    if (f.is_null()) return make_error("Cannot open file: " + path);
    f->store_string(source);
    notify_file_changed(path);
    Dictionary r; r["path"] = path; r["bytes"] = (int64_t)source.length(); return r;
}
Dictionary cmd_validate_gdscript(const Dictionary &a) {
    String path = args_string(a, "path");
    if (!FileAccess::file_exists(path)) return make_error("File not found: " + path);
    String source = FileAccess::get_file_as_string(path);
    String project_root = globalize_path("res://");
    String file_abs = globalize_path(path);
    String file_uri = String("file:///") + file_abs.replace("\\", "/");
    String root_uri = String("file:///") + project_root.replace("\\", "/");

    // Read LSP port from editor settings
    int64_t port = 6005;
    Ref<EditorSettings> es = EditorInterface::get_singleton()->get_editor_settings();
    if (es.is_valid()) {
        Variant v = es->get_setting("network/language_server/remote_port");
        if (v.get_type() != Variant::NIL) {
            port = (int64_t)v;
        }
    }

    // Try LSP first for detailed diagnostics (line, column, severity, message)
    LspClient client;
    Dictionary result = client.validate(path, source, file_uri, root_uri, (uint16_t)port, 5000);
    if (result.get("ok", false)) {
        result["path"] = path;
        return result;
    }

    // LSP failed — fall back to GDScript::reload() for basic ok/fail check
    Ref<GDScript> script;
    script.instantiate();
    script->set_path(path);
    script->set_source_code(source);
    Error err = script->reload();

    Dictionary r;
    r["path"] = path;
    r["ok"] = (err == OK);
    if (err != OK) {
        r["diagnostics"] = Array();
        String lsp_err = result.has("error") ? String(result["error"]) : String("LSP unavailable");
        r["note"] = "LSP connection failed (" + lsp_err + "). Fell back to compile check only — no detailed errors available. The LSP server starts automatically with the Godot editor on port " + String::num_int64(port) + ".";
    }
    return r;
}
void list_gd_rec(const String &dir, bool inc_addons, int64_t max, Array &out) {
    if (out.size() >= max) return;
    Ref<DirAccess> d = DirAccess::open(dir); if (d.is_null()) return;
    d->list_dir_begin();
    while (true) {
        String n = d->get_next(); if (n.is_empty()) break;
        if (n == "." || n == "..") continue;
        String full = dir == "res://" ? String("res://") + n : dir + String("/") + n;
        if (d->current_is_dir()) {
            if (!inc_addons && (n == "addons" || n == ".godot" || n == ".import")) continue;
            list_gd_rec(full, inc_addons, max, out);
        } else if (n.ends_with(".gd") && out.size() < max) { Dictionary e; e["path"] = full; e["size"] = FileAccess::get_file_as_bytes(full).size(); out.append(e); }
    }
}
Dictionary cmd_list_gdscripts(const Dictionary &a) {
    String root = args_string(a, "root", "res://");
    bool inc = args_bool(a, "include_addons", false);
    int64_t max = args_int(a, "max_results", 1000);
    Array out; list_gd_rec(root, inc, max, out);
    Dictionary r; r["scripts"] = out; r["count"] = (int64_t)out.size(); r["truncated"] = out.size() >= max; return r;
}
} // namespace

void register_script_gd(HandlerRegistry &reg) {
    reg.register_tool("create_gdscript", cmd_create_gdscript);
    reg.register_tool("read_gdscript", cmd_read_gdscript);
    reg.register_tool("edit_gdscript", cmd_edit_gdscript);
    reg.register_tool("validate_gdscript", cmd_validate_gdscript);
    reg.register_tool("list_gdscripts", cmd_list_gdscripts);
}
} // namespace godot_mcp