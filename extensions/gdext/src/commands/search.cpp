// commands/search.cpp — find_in_file, search_project, find_and_replace

#include "cmd_utils.hpp"
#include "handler_registry.hpp"
#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/reg_ex.hpp>
#include <godot_cpp/classes/reg_ex_match.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
using namespace godot;

namespace godot_mcp {
namespace {

Array match_lines(const String &source, const String &pattern, bool literal, bool case_sensitive, int64_t max) {
    Array out;
    String pat = pattern;
    if (literal) {
        String escaped;
        for (int i = 0; i < pat.length(); i++) {
            char32_t c = pat[i];
            if (c == '.' || c == '*' || c == '+' || c == '?' || c == '(' || c == ')' ||
                c == '[' || c == ']' || c == '{' || c == '}' || c == '\\' || c == '|' ||
                c == '^' || c == '$')
                escaped += String("\\");
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
            if (text.length() > 240) text = text.substr(0, 240) + String("...");
            Dictionary e; e["line"] = (int64_t)(i + 1); e["col"] = (int64_t)(m->get_start() + 1); e["text"] = text;
            out.append(e);
        }
    }
    return out;
}
void walk_dir(const String &dir, const Array &exts, bool inc_addons, int64_t max, Array &out) {
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
String read_file_text(const String &path) { return FileAccess::file_exists(path) ? FileAccess::get_file_as_string(path) : String(); }

Dictionary cmd_find_in_file(const Dictionary &a) {
    String path = args_string(a, "path"), pattern = args_string(a, "pattern");
    if (pattern.is_empty()) return make_error("missing 'pattern'");
    bool lit = args_bool(a, "literal", false);
    int64_t max = args_int(a, "max_matches", 200);
    String src = read_file_text(path);
    if (src.is_empty() && !FileAccess::file_exists(path)) return make_error("File not found: " + path);
    Array matches = match_lines(src, pattern, lit, true, max);
    Dictionary r; r["path"] = path; r["matches"] = matches; r["count"] = (int64_t)matches.size(); return r;
}
Dictionary cmd_search_project(const Dictionary &a) {
    String pattern = args_string(a, "pattern");
    if (pattern.is_empty()) return make_error("missing 'pattern'");
    bool lit = args_bool(a, "literal", false);
    Array exts;
    if (a.has("extensions") && a["extensions"].get_type() == Variant::ARRAY) exts = a["extensions"];
    else { exts.append(String("gd")); exts.append(String("cs")); exts.append(String("tscn")); exts.append(String("tres")); }
    String root = args_string(a, "root", "res://");
    bool inc = args_bool(a, "include_addons", false);
    int64_t max_m = args_int(a, "max_matches", 500), max_f = args_int(a, "max_files", 5000);
    Array files; walk_dir(root, exts, inc, max_f, files);
    Array all; int64_t scanned = 0;
    for (int i = 0; i < files.size() && all.size() < max_m; i++) {
        String src = read_file_text(files[i]);
        if (!src.is_empty()) { scanned++; Array hits = match_lines(src, pattern, lit, true, max_m - all.size());
            for (int j = 0; j < hits.size(); j++) { Dictionary h = hits[j]; h["path"] = files[i]; all.append(h); } }
    }
    Dictionary r; r["matches"] = all; r["count"] = (int64_t)all.size(); r["files_scanned"] = scanned; r["truncated"] = all.size() >= max_m || files.size() >= max_f; return r;
}
Dictionary cmd_find_and_replace(const Dictionary &a) {
    String path = args_string(a, "path"), pattern = args_string(a, "pattern"), repl = args_string(a, "replacement");
    if (pattern.is_empty()) return make_error("missing 'pattern'");
    bool lit = args_bool(a, "literal", true);
    String src = read_file_text(path);
    if (src.is_empty() && !FileAccess::file_exists(path)) return make_error("File not found: " + path);
    int64_t count = 0;
    String result;
    if (lit) {
        result = src.replace(pattern, repl);
        int idx = 0; while ((idx = src.find(pattern, idx)) >= 0) { count++; idx += pattern.length(); }
    } else {
        return make_error("Regex replace not supported yet; use literal=true");
    }
    if (count == 0) { Dictionary r; r["path"] = path; r["replacements"] = (int64_t)0; r["hint"] = "No matches found"; return r; }
    Ref<FileAccess> f = FileAccess::open(path, FileAccess::WRITE);
    if (f.is_null()) return make_error("Failed to open '" + path + "' for writing");
    f->store_string(result);
    notify_file_changed(path);
    Dictionary r; r["path"] = path; r["replacements"] = count; r["truncated"] = count >= args_int(a, "max_replacements", 10000); return r;
}
} // namespace

void register_search(HandlerRegistry &reg) {
    reg.register_tool("find_in_file", cmd_find_in_file);
    reg.register_tool("search_project", cmd_search_project);
    reg.register_tool("find_and_replace", cmd_find_and_replace);
}
} // namespace godot_mcp