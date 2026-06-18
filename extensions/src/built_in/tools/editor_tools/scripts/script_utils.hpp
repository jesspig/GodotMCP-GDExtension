#pragma once

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot_mcp {

enum class ScriptLang { GDScript, CSharp };

namespace script_utils {

using godot::EditorInterface;
using godot::FileAccess;
using godot::ProjectSettings;
using godot::String;

inline bool has_dotnet() {
    auto *ps = godot::ProjectSettings::get_singleton();
    if (!ps) return false;
    return ps->has_setting("dotnet/project/assembly_name");
}

inline String detect_language(const String &path) {
    if (path.ends_with(".gd")) return "gdscript";
    if (path.ends_with(".cs") || path.ends_with(".csharp")) return "csharp";
    return String();
}

inline String get_file_base_name(const String &path) {
    int slash = path.rfind("/");
    String name = (slash >= 0) ? path.substr(slash + 1) : path;
    int dot = name.rfind(".");
    return (dot >= 0) ? name.substr(0, dot) : name;
}

inline String sanitize_class_name(const String &name) {
    String result;
    for (int i = 0; i < name.length(); i++) {
        char32_t c = name.unicode_at(i);
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') || c == '_') {
            result += String::chr(c);
        }
    }
    if (result.is_empty()) result = "ScriptClass";
    if (result.unicode_at(0) >= '0' && result.unicode_at(0) <= '9') {
        result = String("_") + result;
    }
    return result;
}

} // namespace script_utils
} // namespace godot_mcp
