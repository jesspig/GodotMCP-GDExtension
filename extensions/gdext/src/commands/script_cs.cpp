// commands/script_cs.cpp — C# script tools

#include "cmd_utils.hpp"
#include "handler_registry.hpp"
#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/variant/array.hpp>
#include <cstdio>
#include <random>
using namespace godot;

namespace godot_mcp {
namespace {

String class_name_from_path(const String &path) {
    int slash = path.rfind("/");
    String file = slash >= 0 ? path.substr(slash + 1) : path;
    return file.ends_with(".cs") ? file.substr(0, file.length() - 3) : file;
}
String find_csproj() {
    Ref<DirAccess> d = DirAccess::open("res://"); if (d.is_null()) return "";
    d->list_dir_begin();
    while (true) {
        String n = d->get_next(); if (n.is_empty()) break;
        if (n == "." || n == "..") continue;
        if (!d->current_is_dir() && n.ends_with(".csproj")) return "res://" + n;
    }
    return "";
}
void list_cs_rec(const String &dir, bool inc_addons, int64_t max, Array &out) {
    if (out.size() >= max) return;
    Ref<DirAccess> d = DirAccess::open(dir); if (d.is_null()) return;
    d->list_dir_begin();
    while (true) {
        String n = d->get_next(); if (n.is_empty()) break;
        if (n == "." || n == "..") continue;
        String full = dir == "res://" ? String("res://") + n : dir + String("/") + n;
        if (d->current_is_dir()) {
            if (!inc_addons && (n == "addons" || n == ".godot" || n == ".import")) continue;
            list_cs_rec(full, inc_addons, max, out);
        } else if (n.ends_with(".cs") && out.size() < max) {
            Dictionary e; e["path"] = full; e["size"] = (int64_t)FileAccess::get_file_as_bytes(full).size(); out.append(e);
        }
    }
}
String sanitize_identifier(const String &name) {
    if (name.is_empty()) return "_";
    String r;
    for (int i = 0; i < name.length(); i++) {
        char32_t ch = name[i];
        if (r.is_empty()) { r += (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '_' ? String::chr(ch) : String("_"); }
        else r += (ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '_' || ch == '.' ? String::chr(ch) : String("_");
    }
    return r;
}
String generate_uuid() {
    static std::mt19937_64 rng(std::random_device{}());
    static std::uniform_int_distribution<uint64_t> dist;
    char buf[37];
    const uint64_t hi = dist(rng);
    const uint64_t lo = dist(rng);
    std::sprintf(buf, "%08x-%04x-4%03x-%04x-%012llx",
                 (unsigned)(hi >> 32),
                 (unsigned)(hi >> 16) & 0xffff,
                 (unsigned)(hi & 0x0fff),
                 (unsigned)(lo >> 48) & 0x3fff | 0x8000,
                 (unsigned long long)(lo & 0x0000ffffffffffffULL));
    return String(buf);
}
String get_sdk_version() {
    Dictionary vi = Engine::get_singleton()->get_version_info();
    int64_t major = (int64_t)vi.get("major", 4), minor = (int64_t)vi.get("minor", 0), patch = (int64_t)vi.get("patch", 0);
    return String::num_int64(major) + "." + String::num_int64(minor) + "." + String::num_int64(patch);
}
String get_project_name() {
    ProjectSettings *ps = ProjectSettings::get_singleton();
    Variant v = ps->get_setting("dotnet/project/assembly_name");
    String s = (String)v; if (!s.is_empty()) return s;
    s = (String)ps->get_setting("application/config/name");
    if (!s.is_empty()) return s;
    return "GodotProject";
}

Dictionary cmd_create_csharp_script(const Dictionary &a) {
    String path = args_string(a, "path");
    String base = args_string(a, "base_class", "Node");
    bool overwrite = args_bool(a, "overwrite", false);
    if (path.is_empty()) return make_error("missing 'path'");
    if (!path.ends_with(".cs")) return make_error("path must end with .cs");
    if (find_csproj().is_empty()) return make_error("No .csproj found; call csharp_create_solution first");
    if (FileAccess::file_exists(path) && !overwrite) return make_error("File exists; set overwrite=true");
    ensure_parent_dir(path);
    String cn = class_name_from_path(path);
    String body = "using Godot;\nusing System;\n\npublic partial class " + cn + " : " + base +
        "\n{\n    public override void _Ready()\n    {\n        base._Ready();\n    }\n\n    public override void _Process(double delta)\n    {\n        base._Process(delta);\n    }\n}\n";
    Ref<FileAccess> f = FileAccess::open(path, FileAccess::WRITE);
    if (f.is_null()) return make_error("Cannot open file: " + path);
    f->store_string(body);
    notify_file_changed(path);
    Dictionary r; r["path"] = path; r["created"] = true; r["bytes"] = (int64_t)body.length(); r["language"] = "csharp"; r["class_name"] = cn; return r;
}
Dictionary cmd_read_csharp_script(const Dictionary &a) {
    String path = args_string(a, "path");
    if (!FileAccess::file_exists(path)) return make_error("File not found: " + path);
    String content = FileAccess::get_file_as_string(path);
    Dictionary r; r["path"] = path; r["source"] = content; r["length"] = (int64_t)content.length(); r["language"] = "csharp"; return r;
}
Dictionary cmd_edit_csharp_script(const Dictionary &a) {
    String path = args_string(a, "path"), source = args_string(a, "source");
    if (!path.ends_with(".cs")) return make_error("path must end with .cs");
    if (!FileAccess::file_exists(path)) return make_error("File not found: " + path);
    if (source.is_empty()) return make_error("missing 'source'");
    Ref<FileAccess> f = FileAccess::open(path, FileAccess::WRITE);
    if (f.is_null()) return make_error("Cannot open file: " + path);
    f->store_string(source);
    notify_file_changed(path);
    Dictionary r; r["path"] = path; r["bytes"] = (int64_t)source.length(); r["updated_file"] = true; r["language"] = "csharp"; r["note"] = "C# requires csharp_build to take effect"; return r;
}
Dictionary cmd_list_csharp_scripts(const Dictionary &a) {
    String root = args_string(a, "root", "res://");
    bool inc = args_bool(a, "include_addons", false);
    int64_t max = args_int(a, "max_results", 1000);
    Array out; list_cs_rec(root, inc, max, out);
    Dictionary r; r["scripts"] = out; r["count"] = (int64_t)out.size(); r["truncated"] = out.size() >= max; return r;
}
Dictionary cmd_csharp_build(const Dictionary &a) {
    String cfg = args_string(a, "configuration", "Debug");
    String proot = globalize_path("res://");
    if (proot.is_empty()) return make_error("Cannot resolve project root");
    // Call dotnet build via OS::execute
    PackedStringArray pargs; pargs.append("build"); pargs.append("--nologo"); pargs.append("--configuration"); pargs.append(cfg);
    Array output;
    int32_t exit_code = OS::get_singleton()->execute("dotnet", pargs, output, true);
    Dictionary r;
    if (exit_code == 0) { r["exit_code"] = (int64_t)0; r["success"] = true; r["stdout"] = output.size() > 0 ? (String)output[0] : String(""); }
    else { r["success"] = false; r["error"] = "dotnet build failed with code " + String::num_int64(exit_code); }
    r["project_root"] = proot; r["configuration"] = cfg; return r;
}
Dictionary cmd_csharp_create_solution(const Dictionary &a) {
    bool aot = args_bool(a, "enable_nativeaot", false);
    String proot = globalize_path("res://");
    if (proot.is_empty()) return make_error("Cannot resolve project root");
    String pn = get_project_name(), sv = get_sdk_version(), guid = generate_uuid();
    String sp_n = sanitize_identifier(pn);
    String csproj_f = sp_n + ".csproj", sln_f = sp_n + ".sln";
    String csproj_p = "res://" + csproj_f, sln_p = "res://" + sln_f;
    if (FileAccess::file_exists(csproj_p) || FileAccess::file_exists(sln_p))
        return make_error("Solution files already exist: " + csproj_f + ", " + sln_f);
    String csproj_content = "<Project Sdk=\"Godot.NET.Sdk/" + sv + "\">\n  <PropertyGroup>\n    <TargetFramework>net8.0</TargetFramework>\n    <TargetFramework Condition=\" '$(GodotTargetPlatform)' == 'android' \">net9.0</TargetFramework>\n    <EnableDynamicLoading>true</EnableDynamicLoading>\n" +
        (sp_n != pn ? "    <RootNamespace>" + sp_n + "</RootNamespace>\n" : "") +
        (aot ? "    <PublishAot>true</PublishAot>\n" : "") +
        "  </PropertyGroup>" +
        (aot ? "\n  <ItemGroup>\n    <TrimmerRootAssembly Include=\"GodotSharp\" />\n    <TrimmerRootAssembly Include=\"$(TargetName)\" />\n  </ItemGroup>" : "") +
        "\n</Project>\n";
    // SLN generation
    String sln_content =
        "Microsoft Visual Studio Solution File, Format Version 12.00\n# Visual Studio 2012\n"
        "Project(\"{FAE04EC0-301F-11D3-BF4B-00C04F79EFBC}\") = \"" + sp_n + "\", \"" + csproj_f + "\", \"{" + guid + "}\"\nEndProject\n"
        "Global\n\tGlobalSection(SolutionConfigurationPlatforms) = preSolution\n"
        "\tDebug|Any CPU = Debug|Any CPU\n\tExportDebug|Any CPU = ExportDebug|Any CPU\n\tExportRelease|Any CPU = ExportRelease|Any CPU\n"
        "\tEndGlobalSection\n"
        "\tGlobalSection(ProjectConfigurationPlatforms) = postSolution\n"
        "\t{" + guid + "}.Debug|Any CPU.ActiveCfg = Debug|Any CPU\n\t{" + guid + "}.Debug|Any CPU.Build.0 = Debug|Any CPU\n"
        "\t{" + guid + "}.ExportDebug|Any CPU.ActiveCfg = ExportDebug|Any CPU\n\t{" + guid + "}.ExportDebug|Any CPU.Build.0 = ExportDebug|Any CPU\n"
        "\t{" + guid + "}.ExportRelease|Any CPU.ActiveCfg = ExportRelease|Any CPU\n\t{" + guid + "}.ExportRelease|Any CPU.Build.0 = ExportRelease|Any CPU\n"
        "\tEndGlobalSection\nEndGlobal\n";
    Ref<FileAccess> f;
    f = FileAccess::open(csproj_p, FileAccess::WRITE); if (f.is_null()) return make_error("Failed to write " + csproj_f);
    f->store_string(csproj_content); f.unref();
    f = FileAccess::open(sln_p, FileAccess::WRITE); if (f.is_null()) return make_error("Failed to write " + sln_f);
    f->store_string(sln_content); f.unref();
    notify_file_changed(csproj_p); notify_file_changed(sln_p);
    Dictionary r; r["success"] = true; r["project_name"] = pn; r["sdk_version"] = sv;
    r["guid"] = guid; r["csproj"] = csproj_f; r["sln"] = sln_f; r["enable_nativeaot"] = aot; r["project_root"] = proot; return r;
}
} // namespace

void register_script_cs(HandlerRegistry &reg) {
    reg.register_tool("create_csharp_script", cmd_create_csharp_script);
    reg.register_tool("read_csharp_script", cmd_read_csharp_script);
    reg.register_tool("edit_csharp_script", cmd_edit_csharp_script);
    reg.register_tool("list_csharp_scripts", cmd_list_csharp_scripts);
    reg.register_tool("csharp_build", cmd_csharp_build);
    reg.register_tool("csharp_create_solution", cmd_csharp_create_solution);
}
} // namespace godot_mcp