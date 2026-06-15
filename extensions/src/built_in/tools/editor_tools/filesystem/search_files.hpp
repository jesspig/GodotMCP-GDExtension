#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "filesystem_utils.hpp"

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/editor_file_system.hpp>
#include <godot_cpp/classes/editor_file_system_directory.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/variant/array.hpp>

namespace godot_mcp {

class SearchFilesTool : public ITool {
public:
    String name() const override { return "search_files"; }
    String category() const override { return "editor_tools/filesystem"; }
    String brief() const override {
        return "Search for project files";
    }
    String description() const override {
        return "Searches for files in the project, supporting multiple match modes. "
               "Search scope is limited to res://. "
               "Match modes: exact, substring, glob, regex, fuzzy.";
    }
    Dictionary build_input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Search pattern (empty = list all files)";
            props["pattern"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Optional: search root directory (default res://)";
            props["root"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Match mode: exact | substring | glob | regex | fuzzy (default substring)";
            props["mode"] = p;
        }
        {
            Dictionary p;
            p["type"] = "array";
            p["items"] = Dictionary();
            p["description"] = "Optional: filter extension list, e.g. ['gd','tscn']";
            props["extensions"] = p;
        }
        {
            Dictionary p;
            p["type"] = "boolean";
            p["description"] = "Optional: include addons directory (default false)";
            props["include_addons"] = p;
        }
        {
            Dictionary p;
            p["type"] = "boolean";
            p["description"] = "Optional: case sensitive (default false)";
            props["case_sensitive"] = p;
        }
        {
            Dictionary p;
            p["type"] = "integer";
            p["description"] = "Optional: maximum results (default 200)";
            props["max_results"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        return s;
    }

protected:
    using MatchFn = bool (*)(const String &, const String &, bool);

    static MatchFn get_matcher(const String &mode) {
        if (mode == "exact") return fs_utils::match_exact;
        if (mode == "glob") return fs_utils::match_glob;
        if (mode == "regex") return fs_utils::match_regex;
        if (mode == "fuzzy") return fs_utils::match_fuzzy;
        return fs_utils::match_substring; // default
    }

    static void collect_files(godot::EditorFileSystemDirectory *dir,
                              const String &pattern,
                              MatchFn matcher,
                              bool case_sensitive,
                              const Array &extensions,
                              int64_t max_results,
                              Array &out) {
        if (out.size() >= max_results) return;
        if (!dir) return;

        // Check files in this directory
        for (int i = 0; i < dir->get_file_count() && out.size() < max_results; i++) {
            String name = dir->get_file(i);
            String full_path = dir->get_file_path(i);

            // Extension filter
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

            // Pattern match
            if (!pattern.is_empty()) {
                String match_name = case_sensitive ? name : name.to_lower();
                if (!matcher(match_name, pattern, case_sensitive)) continue;
            }

            Dictionary entry;
            entry["path"] = full_path;
            entry["name"] = name;
            entry["type"] = dir->get_file_type(i);
            entry["script_class"] = dir->get_file_script_class_name(i);
            out.append(entry);
        }

        // Recurse into subdirectories
        for (int i = 0; i < dir->get_subdir_count() && out.size() < max_results; i++) {
            collect_files(dir->get_subdir(i), pattern, matcher,
                          case_sensitive, extensions, max_results, out);
        }
    }

    Dictionary execute_impl(const ToolContext &ctx) override {
        String pattern = args_string(ctx.args, "pattern");
        String root_path = args_string(ctx.args, "root", "res://");
        String mode = args_string(ctx.args, "mode", "substring");
        Array extensions = ctx.args.has("extensions")
            ? static_cast<Array>(ctx.args["extensions"] ): Array();
        bool include_addons = args_bool(ctx.args, "include_addons", false);
        bool case_sensitive = args_bool(ctx.args, "case_sensitive", false);
        int64_t max_results = args_int(ctx.args, "max_results", 200);

        Dictionary verr = fs_utils::validate_res_path(root_path);
        if (!verr.is_empty()) {
            return ToolResult::err(verr["code"], verr["message"]);
        }

        // Validate mode
        Array valid_modes = Array::make("exact", "substring", "glob", "regex", "fuzzy");
        bool mode_ok = false;
        for (int i = 0; i < valid_modes.size(); i++) {
            if (mode == String(valid_modes[i])) { mode_ok = true; break; }
        }
        if (!mode_ok) {
            return ToolResult::err("INVALID_MODE",
                "Invalid match mode, supported: exact/substring/glob/regex/fuzzy");
        }

        // Normalize pattern for case-insensitive
        String search_pattern = case_sensitive ? pattern : pattern.to_lower();
        MatchFn matcher = get_matcher(mode);

        // Use EditorFileSystem for structured traversal
        godot::EditorInterface *ei = godot::EditorInterface::get_singleton();
        Array results;
        if (ei) {
            godot::EditorFileSystem *efs = ei->get_resource_filesystem();
            if (efs) {
                godot::EditorFileSystemDirectory *start_dir;
                if (root_path == "res://") {
                    start_dir = efs->get_filesystem();
                } else {
                    start_dir = efs->get_filesystem_path(root_path);
                }

                if (start_dir) {
                    collect_files(start_dir, search_pattern, matcher,
                                  case_sensitive, extensions, max_results, results);
                }
            }
        }

        // Fallback: use DirAccess if EditorFileSystem not available or gave no results
        if (results.size() == 0) {
            Array ext_filters;
            for (int i = 0; i < extensions.size(); i++) {
                ext_filters.append(String(".") + String(extensions[i]));
            }
            Array all_files;
            walk_project_dir(root_path, ext_filters, include_addons, max_results, all_files);

            for (int i = 0; i < all_files.size() && results.size() < max_results; i++) {
                String full_path = all_files[i];
                String name = fs_utils::get_file_name(full_path);

                if (!pattern.is_empty()) {
                    String match_name = case_sensitive ? name : name.to_lower();
                    if (!matcher(match_name, pattern, case_sensitive)) continue;
                }

                Dictionary entry;
                entry["path"] = full_path;
                entry["name"] = name;
                entry["type"] = String();
                results.append(entry);
            }
        }

        bool truncated = results.size() >= max_results;

        Dictionary data;
        data["files"] = results;
        data["total"] = static_cast<int64_t>(results.size());
        data["truncated"] = truncated;
        data["mode"] = mode;
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp

