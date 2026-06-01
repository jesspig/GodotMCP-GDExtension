// @tool register
// @source builtin
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_settings.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/gd_script.hpp>
#include "lsp/client.hpp"

namespace godot_mcp {

class ValidateGDScriptTool : public ITool {
public:
    String name() const override { return "validate_gdscript"; }
    String category() const override { return "script_gd"; }
    String brief() const override { return "Validate a GDScript file via LSP"; }
    String description() const override { return "Validates a .gd file using the built-in LSP (port 6005) or falls back to GDScript::reload()."; }
    Dictionary input_schema() const override {
        Dictionary s; s["type"] = "object"; Dictionary p;
        p["path"] = [](){Dictionary d;d["type"]="string";d["description"]="Path to .gd file";return d;}();
        s["properties"] = p; Array r; r.push_back("path"); s["required"] = r; return s;
    }
protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String path = args_string(ctx.args, "path");
        if (!FileAccess::file_exists(path)) return ToolResult::err("NOT_FOUND", "File not found: " + path);
        String source = FileAccess::get_file_as_string(path);
        String project_root = globalize_path("res://");
        String file_abs = globalize_path(path);
        String file_uri = String("file:///") + file_abs.replace("\\", "/");
        String root_uri = String("file:///") + project_root.replace("\\", "/");
        int64_t port = 6005;
        Ref<EditorSettings> es = EditorInterface::get_singleton()->get_editor_settings();
        if (es.is_valid()) { Variant v = es->get_setting("network/language_server/remote_port"); if (v.get_type() != Variant::NIL) port = (int64_t)v; }
        LspClient client;
        Dictionary result = client.validate(path, source, file_uri, root_uri, (uint16_t)port, 5000);
        if (result.get("ok", false)) { result["path"] = path; return ToolResult::ok(result); }
        Ref<GDScript> script; script.instantiate();
        script->set_path(path); script->set_source_code(source);
        Error err = script->reload();
        Dictionary d; d["path"] = path; d["ok"] = (err == OK);
        if (err != OK) {
            d["diagnostics"] = Array();
            String lsp_err = result.has("error") ? String(result["error"]) : String("LSP unavailable");
            d["note"] = "LSP connection failed (" + lsp_err + "). Fell back to compile check only.";
        }
        return ToolResult::ok(d);
    }
};

} // namespace godot_mcp
