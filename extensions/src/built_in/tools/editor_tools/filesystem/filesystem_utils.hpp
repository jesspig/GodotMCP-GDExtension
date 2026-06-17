#pragma once

#include <filesystem>
#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/editor_file_system.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/reg_ex.hpp>
#include <godot_cpp/classes/reg_ex_match.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot_mcp::fs_utils {
namespace fs = std::filesystem;

using godot::Array;
using godot::Dictionary;
using godot::DirAccess;
using godot::EditorFileSystem;
using godot::EditorInterface;
using godot::Error;
using godot::FileAccess;
using godot::PackedStringArray;
using godot::ProjectSettings;
using godot::Ref;
using godot::RegEx;
using godot::String;

inline String get_file_extension(const String &path) {
    int dot = path.rfind(".");
    if (dot < 0) return String();
    return path.substr(dot + 1).to_lower();
}

// Check if a file name's extension matches any in the extensions list.
// Extensions are compared case-insensitively without leading dots.
// Returns true if extensions list is empty (no filter).
inline bool matches_extension(const String &name, const Array &extensions) {
    if (extensions.size() <= 0) return true;
    String ext = get_file_extension(name);
    for (int64_t e = 0; e < extensions.size(); e++) {
        if (ext == String(extensions[e]).to_lower()) return true;
    }
    return false;
}

inline bool path_exists(const String &res_path) {
    godot::Ref<godot::DirAccess> dir = godot::DirAccess::open(res_path);
    if (dir.is_valid()) {
        // If we can open it as a directory, it exists
        dir->list_dir_end(); // clean up if opened
        return true;
    }
    // Check if it's a file
    return godot::FileAccess::file_exists(res_path);
}

inline bool is_file(const String &res_path) {
    return godot::FileAccess::file_exists(res_path);
}

inline bool is_directory(const String &res_path) {
    godot::Ref<godot::DirAccess> dir = godot::DirAccess::open(res_path);
    if (dir.is_null()) return false;
    // Successfully opened as directory
    return true;
}

inline Dictionary validate_res_path(const String &path) {
    Dictionary err;
    if (path.is_empty()) {
        err["code"] = "MISSING_ARG";
        err["message"] = "Path cannot be empty";
        return err;
    }
    if (!path.begins_with("res://")) {
        err["code"] = "PATH_INVALID";
        err["message"] = "Path must start with res://";
        return err;
    }
    // Path traversal prevention
    if (path.find("..") >= 0) {
        err["code"] = "PATH_TRAVERSAL";
        err["message"] = "Path must not contain '..'";
        return err;
    }
    // Reject Windows absolute paths sneaking in
    if (path.find("\\") >= 0) {
        err["code"] = "PATH_INVALID";
        err["message"] = "Path must not contain backslashes";
        return err;
    }
    // Verify path doesn't escape res://
    auto *ps = godot::ProjectSettings::get_singleton();
    if (ps) {
        String global_res = ps->globalize_path("res://");
        String global_target = ps->globalize_path(path);
        if (!global_target.begins_with(global_res)) {
            err["code"] = "PATH_TRAVERSAL";
            err["message"] = "Path exceeds res:// scope";
            return err;
        }
    }
    return Dictionary(); // empty = valid
}

inline bool ensure_parent_dir(const String &res_path) {
    if (res_path.begins_with("res://") && res_path.find("/", 6) < 0) {
        return true;
    }
    int slash = res_path.rfind("/");
    if (slash <= 0) return true;
    String parent = res_path.substr(0, slash);
    if (godot::DirAccess::dir_exists_absolute(parent)) return true;
    return godot::DirAccess::make_dir_recursive_absolute(parent) == Error::OK;
}

inline void notify_fs_changes() {
    auto *ei = godot::EditorInterface::get_singleton();
    if (!ei) return;
    auto *efs = ei->get_resource_filesystem();
    if (efs) {
        efs->scan();
    }
}

inline void notify_file_changed(const String &path) {
    auto *ei = godot::EditorInterface::get_singleton();
    if (!ei) return;
    auto *efs = ei->get_resource_filesystem();
    if (efs) {
        efs->update_file(path);
    }
}

inline String get_file_name(const String &path) {
    int slash = path.rfind("/");
    if (slash < 0) return path;
    return path.substr(slash + 1);
}

inline String get_file_base_name(const String &path) {
    String name = get_file_name(path);
    int dot = name.rfind(".");
    if (dot < 0) return name;
    return name.substr(0, dot);
}

// -- Matching helpers for search --

inline bool match_exact(const String &name, const String &pattern, bool case_sensitive) {
    if (case_sensitive) return name == pattern;
    return name.to_lower() == pattern.to_lower();
}

inline bool match_substring(const String &name, const String &pattern, bool case_sensitive) {
    if (case_sensitive) return name.find(pattern) >= 0;
    return name.to_lower().find(pattern.to_lower()) >= 0;
}

inline bool match_glob(const String &name, const String &pattern, bool case_sensitive) {
    if (case_sensitive) return name.match(pattern);
    return name.to_lower().match(pattern.to_lower());
}

inline bool match_regex(const String &name, const String &pattern, bool case_sensitive) {
    godot::Ref<godot::RegEx> regex;
    if (case_sensitive) {
        regex = godot::RegEx::create_from_string(pattern);
    } else {
        regex = godot::RegEx::create_from_string(pattern.to_lower());
    }
    if (regex.is_null()) return false;
    return regex->search(name).is_valid();
}

inline bool match_fuzzy(const String &name, const String &pattern, bool case_sensitive) {
    String n = case_sensitive ? name : name.to_lower();
    String p = case_sensitive ? pattern : pattern.to_lower();
    int pi = 0;
    for (int ni = 0; ni < n.length() && pi < p.length(); ni++) {
        if (n[ni] == p[pi]) pi++;
    }
    return pi == p.length();
}

// -- Recursive directory deletion --

inline Error remove_recursive(const String &res_path) {
    String abs = ProjectSettings::get_singleton()->globalize_path(res_path);
    fs::path p(abs.utf8().get_data());
    std::error_code ec;
    fs::remove_all(p, ec);
    return ec ? Error::FAILED : Error::OK;
}

inline Error copy_recursive(const String &src, const String &dst) {
    String abs_src = ProjectSettings::get_singleton()->globalize_path(src);
    String abs_dst = ProjectSettings::get_singleton()->globalize_path(dst);
    fs::path p_src(abs_src.utf8().get_data());
    fs::path p_dst(abs_dst.utf8().get_data());
    std::error_code ec;
    fs::copy(p_src, p_dst, fs::copy_options::recursive | fs::copy_options::overwrite_existing, ec);
    return ec ? Error::FAILED : Error::OK;
}

} // namespace godot_mcp::fs_utils
