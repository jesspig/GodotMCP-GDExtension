// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include <godot_cpp/classes/file_access.hpp>

namespace godot_mcp {

class ReadCSharpScriptTool : public ITool {
public:
    String name() const override { return "read_csharp_script"; }
    String category() const override { return "script/csharp"; }
    String brief() const override { return "Read a C# script file"; }
    String description() const override { return "Reads the full source content of a .cs file."; }
    Dictionary input_schema() const override {
        Dictionary s; s["type"] = "object"; Dictionary p;
        p["path"] = [](){Dictionary d;d["type"]="string";d["description"]="Path to .cs file";return d;}();
        s["properties"] = p; Array r; r.push_back("path"); s["required"] = r; return s;
    }
protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String path = args_string(ctx.args, "path");
        if (!FileAccess::file_exists(path)) return ToolResult::err("NOT_FOUND", "File not found: " + path);
        String content = FileAccess::get_file_as_string(path);
        Dictionary d; d["path"] = path; d["source"] = content; d["length"] = (int64_t)content.length(); d["language"] = "csharp";
        return ToolResult::ok(d);
    }
};

} // namespace godot_mcp
