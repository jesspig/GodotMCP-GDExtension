#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "built_in/tools/editor_tools/scripts/script_utils.hpp"
#include "built_in/tools/editor_tools/filesystem/filesystem_utils.hpp"

#include <godot_cpp/classes/file_access.hpp>

namespace godot_mcp {

class PatchGdScriptTool : public ITool {
public:
    String name() const override { return "patch_gd_script"; }
    String category() const override { return "editor_tools/scripts"; }
    String brief() const override {
        return "Precision replace text in GDScript (.gd) files";
    }
    String description() const override {
        return "Perform precision text replacement in GDScript files. Replaces old_text with new_text, suitable for surgical modifications rather than rewriting the entire file. occurrence=0 replaces all matches, >0 replaces only the Nth occurrence.";
    }
    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Target file path (must end with .gd)";
            props["path"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Old text to be replaced";
            props["old_text"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "New text to replace with";
            props["new_text"] = p;
        }
        {
            Dictionary p;
            p["type"] = "integer";
            p["description"] = "Optional: replace Nth occurrence (0=all, default 0)";
            props["occurrence"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        s["required"] = Array::make("path", "old_text", "new_text");
        return s;
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String path = args_string(ctx.args, "path");
        String old_text = args_string(ctx.args, "old_text");
        String new_text = args_string(ctx.args, "new_text");
        int64_t occurrence = args_int(ctx.args, "occurrence", 0);

        Dictionary verr = fs_utils::validate_res_path(path);
        if (!verr.is_empty()) {
            return ToolResult::err(verr["code"], verr["message"]);
        }
        if (!path.ends_with(".gd")) {
            return ToolResult::err("BAD_EXTENSION",
                "Path must end with .gd");
        }
        if (!FileAccess::file_exists(path)) {
            return ToolResult::err("NOT_FOUND",
                "File does not exist: " + path);
        }
        if (old_text.is_empty()) {
            return ToolResult::err("MISSING_ARG",
                "old_text cannot be empty");
        }

        Ref<FileAccess> file = FileAccess::open(path, FileAccess::READ);
        if (file.is_null()) {
            return ToolResult::err("READ_FAILED",
                "Failed to open file for reading: " + path);
        }
        String content = file->get_as_text();
        file->close();

        int64_t old_size = content.length();
        int64_t replacements = 0;

        if (occurrence <= 0) {
            int64_t count = 0;
            int idx = content.find(old_text);
            while (idx >= 0) {
                count++;
                idx = content.find(old_text, idx + old_text.length());
            }
            content = content.replace(old_text, new_text);
            replacements = count;
        } else {
            int64_t found = 0;
            int idx = content.find(old_text);
            while (idx >= 0 && found < occurrence) {
                found++;
                if (found == occurrence) {
                    content = content.substr(0, idx) + new_text + content.substr(idx + old_text.length());
                    replacements = 1;
                    break;
                }
                idx = content.find(old_text, idx + old_text.length());
            }
        }

        if (replacements == 0) {
            return ToolResult::err("NO_MATCH",
                "No matching text found: " + old_text);
        }

        Ref<FileAccess> wf = FileAccess::open(path, FileAccess::WRITE);
        if (wf.is_null()) {
            return ToolResult::err("WRITE_FAILED",
                "Failed to open file for writing: " + path);
        }
        wf->store_string(content);
        wf->close();

        fs_utils::notify_file_changed(path);

        Dictionary data;
        data["path"] = path;
        data["replacements_made"] = replacements;
        data["old_size"] = old_size;
        data["new_size"] = (int64_t)content.length();
        data["language"] = String("gdscript");
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp

