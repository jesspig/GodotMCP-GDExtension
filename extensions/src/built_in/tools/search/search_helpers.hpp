#pragma once

#include "built_in/cmd_utils.hpp"
#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/reg_ex.hpp>
#include <godot_cpp/classes/reg_ex_match.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>

namespace godot_mcp {

inline Array match_lines(const String &source, const String &pattern, bool literal, int64_t max) {
    Array out;
    String pat = pattern;
    if (literal) {
        String escaped;
        for (int i = 0; i < pat.length(); i++) {
            char32_t c = pat[i];
            if (c == '.' || c == '*' || c == '+' || c == '?' || c == '(' || c == ')' ||
                c == '[' || c == ']' || c == '{' || c == '}' || c == '\\' || c == '|' ||
                c == '^' || c == '$') escaped += "\\";
            escaped += String::chr(c);
        }
        pat = escaped;
    }
    Ref<RegEx> re = RegEx::create_from_string(pat);
    if (re.is_null()) return out;
    PackedStringArray lines = source.split("\n", false);
    for (int i = 0; i < lines.size() && out.size() < max; i++) {
        Ref<RegExMatch> m = re->search(lines[i]);
        if (m.is_valid()) {
            String text = lines[i];
            if (text.length() > 240) text = text.substr(0, 240) + "...";
            Dictionary e; e["line"] = (int64_t)(i + 1); e["col"] = (int64_t)(m->get_start() + 1); e["text"] = text;
            out.append(e);
        }
    }
    return out;
}
inline void walk_dir(const String &dir, const Array &exts, bool inc_addons, int64_t max, Array &out) {
    if (out.size() >= max) return;
    Ref<DirAccess> d = DirAccess::open(dir); if (d.is_null()) return;
    d->list_dir_begin();
    while (true) {
        String n = d->get_next(); if (n.is_empty()) break;
        if (n == "." || n == "..") continue;
        String full = dir == "res://" ? String("res://") + n : dir + String("/") + n;
        if (d->current_is_dir()) {
            if (!inc_addons && (n == "addons" || n == ".godot" || n == ".import")) continue;
            walk_dir(full, exts, inc_addons, max, out);
        } else {
            bool match = exts.size() == 0;
            for (int i = 0; i < exts.size() && !match; i++) { if (n.ends_with((String)exts[i])) match = true; }
            if (match && out.size() < max) out.append(full);
        }
    }
}
inline String read_file_text(const String &path) {
    return FileAccess::file_exists(path) ? FileAccess::get_file_as_string(path) : String();
}

} // namespace godot_mcp
