// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "filesystem_utils.hpp"

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/editor_file_system.hpp>
#include <godot_cpp/classes/editor_file_system_directory.hpp>
#include <godot_cpp/classes/editor_interface.hpp>

namespace godot_mcp {

class ListDirectoryTool : public ITool {
public:
    String name() const override { return "list_directory"; }
    String category() const override { return "editor_tools/filesystem"; }
    String brief() const override {
        return "List directory contents";
    }
    String description() const override {
        return "Lists files and subdirectories in the specified directory. "
               "Prefers EditorFileSystemDirectory for structured information, "
               "falling back to DirAccess for direct traversal. Supports filtering by extension.";
    }
    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Optional: directory path (default res://)";
            props["path"] = p;
        }
        {
            Dictionary p;
            p["type"] = "boolean";
            p["description"] = "Optional: include files (default true)";
            props["include_files"] = p;
        }
        {
            Dictionary p;
            p["type"] = "boolean";
            p["description"] = "Optional: include subdirectories (default true)";
            props["include_dirs"] = p;
        }
        {
            Dictionary p;
            p["type"] = "array";
            p["items"] = Dictionary();
            p["description"] = "Optional: filter extension list, e.g. ['gd', 'tscn']";
            props["extensions"] = p;
        }
        {
            Dictionary p;
            p["type"] = "boolean";
            p["description"] = "Optional: list recursively (default false)";
            props["recursive"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        return s;
    }

protected:
    void collect_dir_efs(EditorFileSystemDirectory *dir,
                         bool include_files, bool include_dirs,
                         const Array &extensions,
                         bool recursive,
                         Array &file_list, Array &dir_list) const {
        if (!dir) return;

        // Files
        if (include_files) {
            for (int i = 0; i < dir->get_file_count(); i++) {
                String name = dir->get_file(i);
                String full_path = dir->get_file_path(i);

                if (extensions.size() > 0) {
                    String ext = fs_utils::get_file_extension(name);
                    bool ext_match = false;
                    for (int e = 0; e < extensions.size(); e++) {
                        if (ext == String(extensions[e]).to_lower()) {
                            ext_match = true;
                            break;
                        }
                    }
                    if (!ext_match) continue;
                }

                Dictionary entry;
                entry["name"] = name;
                entry["path"] = full_path;
                entry["type"] = dir->get_file_type(i);
                file_list.append(entry);
            }
        }

        // Subdirectories
        if (include_dirs) {
            for (int i = 0; i < dir->get_subdir_count(); i++) {
                EditorFileSystemDirectory *sub = dir->get_subdir(i);
                Dictionary entry;
                entry["name"] = sub->get_name();
                entry["path"] = sub->get_path();
                entry["file_count"] = sub->get_file_count();
                dir_list.append(entry);

                if (recursive) {
                    collect_dir_efs(sub, include_files, include_dirs,
                                    extensions, true, file_list, dir_list);
                }
            }
        }
    }

    void collect_dir_raw(const String &dir_path,
                         bool include_files, bool include_dirs,
                         const Array &extensions,
                         bool recursive,
                         Array &file_list, Array &dir_list) const {
        Ref<DirAccess> dir = DirAccess::open(dir_path);
        if (dir.is_null()) return;

        dir->list_dir_begin();
        while (true) {
            String n = dir->get_next();
            if (n.is_empty()) break;
            if (n == "." || n == "..") continue;

            String full = dir_path.ends_with("/") ? dir_path + n : dir_path + String("/") + n;

            if (dir->current_is_dir()) {
                if (include_dirs) {
                    Dictionary entry;
                    entry["name"] = n;
                    entry["path"] = full;
                    dir_list.append(entry);
                }
                if (recursive) {
                    collect_dir_raw(full, include_files, include_dirs,
                                    extensions, true, file_list, dir_list);
                }
            } else {
                if (!include_files) continue;
                if (extensions.size() > 0) {
                    String ext = fs_utils::get_file_extension(n);
                    bool ext_match = false;
                    for (int e = 0; e < extensions.size(); e++) {
                        if (ext == String(extensions[e]).to_lower()) {
                            ext_match = true;
                            break;
                        }
                    }
                    if (!ext_match) continue;
                }
                Dictionary entry;
                entry["name"] = n;
                entry["path"] = full;
                file_list.append(entry);
            }
        }
        dir->list_dir_end();
    }

    Dictionary execute_impl(const ToolContext &ctx) override {
        String path = args_string(ctx.args, "path", "res://");
        bool include_files = args_bool(ctx.args, "include_files", true);
        bool include_dirs = args_bool(ctx.args, "include_dirs", true);
        Array extensions = ctx.args.has("extensions")
            ? (Array)ctx.args["extensions"] : Array();
        bool recursive = args_bool(ctx.args, "recursive", false);

        Dictionary verr = fs_utils::validate_res_path(path);
        if (!verr.is_empty()) {
            return ToolResult::err(verr["code"], verr["message"]);
        }
        if (!fs_utils::is_directory(path)) {
            return ToolResult::err("NOT_A_DIRECTORY",
                "Path is not a directory: " + path);
        }

        Array file_list;
        Array dir_list;

        // Try EditorFileSystem first for structured data
        EditorInterface *ei = EditorInterface::get_singleton();
        bool efs_used = false;
        if (ei) {
            EditorFileSystem *efs = ei->get_resource_filesystem();
            if (efs) {
                EditorFileSystemDirectory *efs_dir;
                if (path == "res://") {
                    efs_dir = efs->get_filesystem();
                } else {
                    efs_dir = efs->get_filesystem_path(path);
                }
                if (efs_dir) {
                    collect_dir_efs(efs_dir, include_files, include_dirs,
                                    extensions, recursive, file_list, dir_list);
                    efs_used = true;
                }
            }
        }

        // Fallback to DirAccess
        if (!efs_used) {
            collect_dir_raw(path, include_files, include_dirs,
                            extensions, recursive, file_list, dir_list);
        }

        Dictionary data;
        data["path"] = path;
        data["files"] = file_list;
        data["directories"] = dir_list;
        data["total_files"] = (int64_t)file_list.size();
        data["total_dirs"] = (int64_t)dir_list.size();
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
