#pragma once

#include "built_in/cmd_utils.hpp"
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

inline String read_file_text(const String &path) {
    return FileAccess::file_exists(path) ? FileAccess::get_file_as_string(path) : String();
}

} // namespace godot_mcp
