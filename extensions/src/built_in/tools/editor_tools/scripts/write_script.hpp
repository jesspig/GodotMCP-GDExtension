#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "built_in/cmd_utils/schema_builder.hpp"
#include "built_in/tools/editor_tools/scripts/script_utils.hpp"
#include "built_in/tools/editor_tools/filesystem/filesystem_utils.hpp"

#include <godot_cpp/classes/file_access.hpp>

namespace godot_mcp {

template <ScriptLang Lang>
class WriteScriptTool : public ITool {
    String ext() const {
        if constexpr (Lang == ScriptLang::GDScript) return ".gd";
        else return ".cs";
    }
    String lang_id() const {
        if constexpr (Lang == ScriptLang::GDScript) return "gdscript";
        else return "csharp";
    }
    String tool_name() const {
        if constexpr (Lang == ScriptLang::GDScript) return "write_gd_script";
        else return "write_csharp_script";
    }

public:
    String name() const noexcept override { return tool_name(); }
    String category() const noexcept override { return "editor_tools/scripts"; }
    String brief() const noexcept override {
        if constexpr (Lang == ScriptLang::GDScript)
            return "Write/Create GDScript (.gd) file";
        else
            return "Write/Create C# Script (.cs) file";
    }
    String description() const override {
        if constexpr (Lang == ScriptLang::GDScript) {
            return "Create or overwrite a GDScript file. Writes directly when content is provided; creates a minimal valid script (extends Node) when content is empty. AI clients should provide complete script content.";
        } else {
            return "Create or overwrite a C# script file. Writes directly when content is provided; creates a minimal valid script when content is empty.";
        }
    }
    Dictionary build_input_schema() const override {
        return SchemaBuilder()
            .prop("path", "string", String("Target path (must end with ") + ext() + String(")"))
            .prop("content", "string", "File content (empty = create template)")
            .required(Array::make("path"))
            .build();
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String path = args_string(ctx.args, "path");
        String content = args_string(ctx.args, "content");

        Dictionary verr = fs_utils::validate_res_path(path);
        if (!verr.is_empty()) {
            return ToolResult::err(verr["code"], verr["message"]);
        }
        if (!path.ends_with(ext())) {
            return ToolResult::err("BAD_EXTENSION",
                String("Path must end with ") + ext());
        }
        if (!fs_utils::ensure_parent_dir(path)) {
            return ToolResult::err("MKDIR_FAILED",
                "Failed to create parent directory");
        }

        if (content.is_empty()) {
            if constexpr (Lang == ScriptLang::GDScript) {
                content = String("extends Node\n");
            } else {
                String class_name = script_utils::sanitize_class_name(script_utils::get_file_base_name(path));
                content = String("using Godot;\n\nnamespace Game;\n\npublic partial class ") + class_name + String(" : Node\n{\n}\n");
            }
        }

        godot::Ref<godot::FileAccess> file = godot::FileAccess::open(path, godot::FileAccess::WRITE);
        if (file.is_null()) {
            return ToolResult::err("WRITE_FAILED",
                "Failed to open file for writing");
        }
        file->store_string(content);
        file->close();

        fs_utils::notify_file_changed(path);

        Dictionary data;
        data["path"] = path;
        data["name"] = fs_utils::get_file_name(path);
        data["language"] = lang_id();
        data["size"] = static_cast<int64_t>(content.length());
        return ToolResult::ok(data);
    }
};

using WriteGdScriptTool = WriteScriptTool<ScriptLang::GDScript>;
using WriteCsharpScriptTool = WriteScriptTool<ScriptLang::CSharp>;

} // namespace godot_mcp
