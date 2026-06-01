// @tool register
// @source builtin
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include <godot_cpp/classes/file_access.hpp>

namespace godot_mcp {

class ReadGDScriptTool : public ITool {
public:
    String name() const override { return "read_gdscript"; }
    String category() const override { return "script_gd"; }
    String brief() const override { return "Read a GDScript file"; }
    String description() const override { return "Reads the full source content of a .gd file."; }
    Dictionary input_schema() const override {
        Dictionary s; s["type"] = "object"; Dictionary p;
        p["path"] = [](){Dictionary d;d["type"]="string";d["description"]="Path to .gd file";return d;}();
        s["properties"] = p; Array r; r.push_back("path"); s["required"] = r; return s;
    }
protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String path = args_string(ctx.args, "path");
        if (!FileAccess::file_exists(path)) return ToolResult::err("NOT_FOUND", "File not found: " + path);
        String content = FileAccess::get_file_as_string(path);
        Dictionary d; d["path"] = path; d["source"] = content; d["length"] = (int64_t)content.length(); d["language"] = "gdscript";
        return ToolResult::ok(d);
    }
};

} // namespace godot_mcp
