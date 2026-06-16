#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "built_in/tools/editor_tools/scripts/script_utils.hpp"
#include "built_in/tools/editor_tools/filesystem/filesystem_utils.hpp"

#include <godot_cpp/classes/file_access.hpp>

namespace godot_mcp {

template <ScriptLang Lang>
class ReadScriptTool : public ITool {
    String ext() const {
        if constexpr (Lang == ScriptLang::GDScript) return ".gd";
        else return ".cs";
    }
    String lang_name() const {
        if constexpr (Lang == ScriptLang::GDScript) return "GDScript";
        else return "C#";
    }
    String lang_id() const {
        if constexpr (Lang == ScriptLang::GDScript) return "gdscript";
        else return "csharp";
    }
    String tool_name() const {
        if constexpr (Lang == ScriptLang::GDScript) return "read_gd_script";
        else return "read_csharp_script";
    }

public:
    String name() const noexcept override { return tool_name(); }
    String category() const noexcept override { return "editor_tools/scripts"; }
    String brief() const noexcept override {
        if constexpr (Lang == ScriptLang::GDScript)
            return "Read GDScript (.gd) file contents";
        else
            return "Read C# Script (.cs) file contents";
    }
    String description() const override {
        return String("Read the contents of a ") + lang_name() + String(" file at the specified path. Returns the file path, full content, line count, and language type.");
    }
    Dictionary build_input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String("Target file path (must end with ") + ext() + String(")");
            props["path"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        s["required"] = Array::make("path");
        return s;
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String path = args_string(ctx.args, "path");

        Dictionary verr = fs_utils::validate_res_path(path);
        if (!verr.is_empty()) {
            return ToolResult::err(verr["code"], verr["message"]);
        }
        if (!path.ends_with(ext())) {
            return ToolResult::err("BAD_EXTENSION",
                String("Path must end with ") + ext());
        }
        if (!godot::FileAccess::file_exists(path)) {
            return ToolResult::err("NOT_FOUND",
                "File does not exist: " + path);
        }

        godot::Ref<godot::FileAccess> file = godot::FileAccess::open(path, godot::FileAccess::READ);
        if (file.is_null()) {
            return ToolResult::err("READ_FAILED",
                "Failed to open file: " + path);
        }
        String content = file->get_as_text();
        file->close();

        int64_t line_count = 1;
        for (int i = 0; i < content.length(); i++) {
            if (content[i] == '\n') line_count++;
        }

        Dictionary data;
        data["path"] = path;
        data["content"] = content;
        data["line_count"] = line_count;
        data["language"] = lang_id();
        return ToolResult::ok(data);
    }
};

using ReadGdScriptTool = ReadScriptTool<ScriptLang::GDScript>;
using ReadCsharpScriptTool = ReadScriptTool<ScriptLang::CSharp>;

} // namespace godot_mcp
