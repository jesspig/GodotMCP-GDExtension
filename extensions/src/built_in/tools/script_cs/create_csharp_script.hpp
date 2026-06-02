// @tool register
// @source builtin
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/tools/script_cs/script_cs_helpers.hpp"
#include <godot_cpp/classes/file_access.hpp>

namespace godot_mcp {

class CreateCSharpScriptTool : public ITool {
public:
    String name() const override { return "create_csharp_script"; }
    String category() const override { return "script_cs"; }
    String brief() const override { return "Create a new C# script file"; }
    String category_description() const override { return String::utf8("C# 脚本的创建与管理"); }
    String description() const override { return "Creates a .cs file with a basic class template. Requires an existing .csproj."; }
    Dictionary input_schema() const override {
        Dictionary s; s["type"] = "object"; Dictionary p;
        p["path"] = [](){Dictionary d;d["type"]="string";d["description"]="Output path (.cs)";return d;}();
        p["base_class"] = [](){Dictionary d;d["type"]="string";d["description"]="Base class (default: Node)";return d;}();
        p["overwrite"] = [](){Dictionary d;d["type"]="boolean";d["description"]="Overwrite existing file";return d;}();
        s["properties"] = p; Array r; r.push_back("path"); s["required"] = r; return s;
    }
protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String path = args_string(ctx.args, "path"); String base = args_string(ctx.args, "base_class", "Node");
        bool overwrite = args_bool(ctx.args, "overwrite", false);
        if (path.is_empty()) return ToolResult::err("MISSING_PARAM", "missing 'path'");
        if (!path.ends_with(".cs")) return ToolResult::err("INVALID_PATH", "path must end with .cs");
        if (find_csproj().is_empty()) return ToolResult::err("NO_CSPROJ", "No .csproj found; call csharp_create_solution first");
        if (FileAccess::file_exists(path) && !overwrite) return ToolResult::err("EXISTS", "File exists; set overwrite=true");
        ensure_parent_dir(path);
        String cn = class_name_from_path(path);
        String body = "using Godot;\nusing System;\n\npublic partial class " + cn + " : " + base +
            "\n{\n    public override void _Ready()\n    {\n        base._Ready();\n    }\n\n    public override void _Process(double delta)\n    {\n        base._Process(delta);\n    }\n}\n";
        Ref<FileAccess> f = FileAccess::open(path, FileAccess::WRITE);
        if (f.is_null()) return ToolResult::err("FILE_ERROR", "Cannot open file: " + path);
        f->store_string(body);
        notify_file_changed(path);
        Dictionary d; d["path"] = path; d["created"] = true; d["bytes"] = (int64_t)body.length(); d["language"] = "csharp"; d["class_name"] = cn;
        return ToolResult::ok(d);
    }
};

} // namespace godot_mcp
