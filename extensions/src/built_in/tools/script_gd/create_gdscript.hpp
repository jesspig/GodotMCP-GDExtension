// @tool register
// @source builtin
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include <godot_cpp/classes/file_access.hpp>

namespace godot_mcp {

class CreateGDScriptTool : public ITool {
public:
    String name() const override { return "create_gdscript"; }
    String category() const override { return "script_gd"; }
    String brief() const override { return "Create a new GDScript file"; }
    String description() const override { return "Creates a .gd file with an optional template or custom content."; }
    Dictionary input_schema() const override {
        Dictionary s; s["type"] = "object"; Dictionary p;
        p["path"] = [](){Dictionary d;d["type"]="string";d["description"]="Output path (.gd)";return d;}();
        p["base_class"] = [](){Dictionary d;d["type"]="string";d["description"]="Base class (default: RefCounted)";return d;}();
        p["class_name"] = [](){Dictionary d;d["type"]="string";d["description"]="Optional class_name declaration";return d;}();
        p["template"] = [](){Dictionary d;d["type"]="string";d["description"]="Template: 'empty' or custom content with _BASE_/_CLASS_ placeholders";return d;}();
        p["overwrite"] = [](){Dictionary d;d["type"]="boolean";d["description"]="Overwrite existing file";return d;}();
        s["properties"] = p; Array r; r.push_back("path"); s["required"] = r; return s;
    }
protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String path = args_string(ctx.args, "path");
        if (path.is_empty()) return ToolResult::err("MISSING_PARAM", "missing 'path'");
        if (!path.ends_with(".gd")) return ToolResult::err("INVALID_PATH", "path must end with .gd");
        bool overwrite = args_bool(ctx.args, "overwrite", false);
        if (FileAccess::file_exists(path) && !overwrite) return ToolResult::err("EXISTS", "File exists; set overwrite=true");
        String base = args_string(ctx.args, "base_class", "RefCounted");
        String cn = args_string(ctx.args, "class_name");
        String template_str = args_string(ctx.args, "template", args_string(ctx.args, "content"));
        bool empty_body = (template_str == "empty");
        String body;
        if (empty_body) {
            body = "extends " + base + "\n" + (cn.is_empty() ? "" : "class_name " + cn + "\n");
        } else if (!template_str.is_empty() && template_str != "empty") {
            body = template_str; body = body.replace("_BASE_", base).replace("_CLASS_", cn);
        } else {
            body = "extends " + base + "\n" + (cn.is_empty() ? "" : "class_name " + cn + "\n") +
                "# Called when the node enters the scene tree.\nfunc _ready() -> void:\n\tpass\n\nfunc _process(delta: float) -> void:\n\tpass\n";
        }
        ensure_parent_dir(path);
        Ref<FileAccess> f = FileAccess::open(path, FileAccess::WRITE);
        if (f.is_null()) return ToolResult::err("FILE_ERROR", "Cannot open file for writing: " + path);
        f->store_string(body);
        notify_file_changed(path);
        Dictionary d; d["path"] = path; d["created"] = true; d["bytes"] = (int64_t)body.length(); d["language"] = "gdscript";
        return ToolResult::ok(d);
    }
};

} // namespace godot_mcp
