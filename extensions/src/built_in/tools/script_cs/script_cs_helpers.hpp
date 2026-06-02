#pragma once

#include "built_in/cmd_utils.hpp"
#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/variant/array.hpp>
#include <cstdio>
#include <random>

namespace godot_mcp {

inline String class_name_from_path(const String &path) {
    int slash = path.rfind("/");
    String file = slash >= 0 ? path.substr(slash + 1) : path;
    return file.ends_with(".cs") ? file.substr(0, file.length() - 3) : file;
}
inline String find_csproj() {
    Ref<DirAccess> d = DirAccess::open("res://"); if (d.is_null()) return "";
    d->list_dir_begin();
    while (true) { String n = d->get_next(); if (n.is_empty()) break; if (n == "." || n == "..") continue; if (!d->current_is_dir() && n.ends_with(".csproj")) return "res://" + n; }
    return "";
}
inline void list_cs_rec(const String &dir, bool inc_addons, int64_t max, Array &out) {
    if (out.size() >= max) return;
    Ref<DirAccess> d = DirAccess::open(dir); if (d.is_null()) return;
    d->list_dir_begin();
    while (true) {
        String n = d->get_next(); if (n.is_empty()) break;
        if (n == "." || n == "..") continue;
        String full = dir == "res://" ? String("res://") + n : dir + String("/") + n;
        if (d->current_is_dir()) { if (!inc_addons && (n == "addons" || n == ".godot" || n == ".import")) continue; list_cs_rec(full, inc_addons, max, out); }
        else if (n.ends_with(".cs") && out.size() < max) { Dictionary e; e["path"] = full; e["size"] = (int64_t)FileAccess::get_file_as_bytes(full).size(); out.append(e); }
    }
}
inline String sanitize_identifier(const String &name) {
    if (name.is_empty()) return "_";
    String r;
    for (int i = 0; i < name.length(); i++) {
        char32_t ch = name[i];
        if (r.is_empty()) r += (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '_' ? String::chr(ch) : String("_");
        else r += (ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '_' || ch == '.' ? String::chr(ch) : String("_");
    }
    return r;
}
inline String generate_uuid() {
    static std::mt19937_64 rng(std::random_device{}());
    static std::uniform_int_distribution<uint64_t> dist;
    char buf[37]; const uint64_t hi = dist(rng); const uint64_t lo = dist(rng);
    std::sprintf(buf, "%08x-%04x-4%03x-%04x-%012llx",
        (unsigned)(hi >> 32), (unsigned)(hi >> 16) & 0xffff, (unsigned)(hi & 0x0fff),
        (unsigned)(lo >> 48) & 0x3fff | 0x8000, (unsigned long long)(lo & 0x0000ffffffffffffULL));
    return String(buf);
}
inline String get_sdk_version() {
    Dictionary vi = Engine::get_singleton()->get_version_info();
    int64_t major = (int64_t)vi.get("major", 4), minor = (int64_t)vi.get("minor", 0), patch = (int64_t)vi.get("patch", 0);
    return String::num_int64(major) + "." + String::num_int64(minor) + "." + String::num_int64(patch);
}
inline String get_project_name() {
    ProjectSettings *ps = ProjectSettings::get_singleton();
    Variant v = ps->get_setting("dotnet/project/assembly_name");
    String s = (String)v; if (!s.is_empty()) return s;
    s = (String)ps->get_setting("application/config/name");
    return s.is_empty() ? "GodotProject" : s;
}

} // namespace godot_mcp
