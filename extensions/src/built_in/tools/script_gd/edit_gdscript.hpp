// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include <godot_cpp/classes/file_access.hpp>

namespace godot_mcp {

class EditGDScriptTool : public ITool {
public:
    String name() const override { return "edit_gdscript"; }
    String category() const override { return "script_gd"; }
    String brief() const override { return "Edit a GDScript file"; }
    String description() const override { return "Writes new source content to a .gd file, overwriting existing content."; }
    Dictionary input_schema() const override {
        Dictionary s; s["type"] = "object"; Dictionary p;
        p["path"] = [](){Dictionary d;d["type"]="string";d["description"]="Path to .gd file";return d;}();
        p["source"] = [](){Dictionary d;d["type"]="string";d["description"]="New source content";return d;}();
        s["properties"] = p; Array r; r.push_back("path"); r.push_back("source"); s["required"] = r; return s;
    }
protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String path = args_string(ctx.args, "path"), source = args_string(ctx.args, "source");
        if (!path.ends_with(".gd")) return ToolResult::err("INVALID_PATH", "path must end with .gd");
        if (!FileAccess::file_exists(path)) return ToolResult::err("NOT_FOUND", "File not found: " + path);
        if (source.is_empty()) return ToolResult::err("MISSING_PARAM", "missing 'source'");
        Ref<FileAccess> f = FileAccess::open(path, FileAccess::WRITE);
        if (f.is_null()) return ToolResult::err("FILE_ERROR", "Cannot open file: " + path);
        f->store_string(source);
        notify_file_changed(path);
        Dictionary d; d["path"] = path; d["bytes"] = (int64_t)source.length();
        return ToolResult::ok(d);
    }
};

} // namespace godot_mcp
