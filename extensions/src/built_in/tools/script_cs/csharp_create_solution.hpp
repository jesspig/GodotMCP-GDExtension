// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/tools/script_cs/script_cs_helpers.hpp"
#include <godot_cpp/classes/file_access.hpp>

namespace godot_mcp {

class CSharpCreateSolutionTool : public ITool {
public:
    String name() const override { return "csharp_create_solution"; }
    String category() const override { return "script/csharp"; }
    String brief() const override { return "Create .csproj and .sln files"; }
    String description() const override { return "Generates a .csproj and .sln for C# support in the project."; }
    Dictionary input_schema() const override {
        Dictionary s; s["type"] = "object"; Dictionary p;
        p["enable_nativeaot"] = [](){Dictionary d;d["type"]="boolean";d["description"]="Enable NativeAOT publishing";return d;}();
        s["properties"] = p; return s;
    }
protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        bool aot = args_bool(ctx.args, "enable_nativeaot", false);
        String proot = globalize_path("res://");
        if (proot.is_empty()) return ToolResult::err("PATH_ERROR", "Cannot resolve project root");
        String pn = get_project_name(), sv = get_sdk_version(), guid = generate_uuid();
        String sp_n = sanitize_identifier(pn);
        String csproj_f = sp_n + ".csproj", sln_f = sp_n + ".sln";
        String csproj_p = "res://" + csproj_f, sln_p = "res://" + sln_f;
        if (FileAccess::file_exists(csproj_p) || FileAccess::file_exists(sln_p))
            return ToolResult::err("EXISTS", "Solution files already exist: " + csproj_f + ", " + sln_f);
        String csproj_content = "<Project Sdk=\"Godot.NET.Sdk/" + sv + "\">\n  <PropertyGroup>\n    <TargetFramework>net8.0</TargetFramework>\n    <TargetFramework Condition=\" '$(GodotTargetPlatform)' == 'android' \">net9.0</TargetFramework>\n    <EnableDynamicLoading>true</EnableDynamicLoading>\n" +
            (sp_n != pn ? "    <RootNamespace>" + sp_n + "</RootNamespace>\n" : "") +
            (aot ? "    <PublishAot>true</PublishAot>\n" : "") +
            "  </PropertyGroup>" +
            (aot ? "\n  <ItemGroup>\n    <TrimmerRootAssembly Include=\"GodotSharp\" />\n    <TrimmerRootAssembly Include=\"$(TargetName)\" />\n  </ItemGroup>" : "") +
            "\n</Project>\n";
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
        f = FileAccess::open(csproj_p, FileAccess::WRITE); if (f.is_null()) return ToolResult::err("FILE_ERROR", "Failed to write " + csproj_f);
        f->store_string(csproj_content); f.unref();
        f = FileAccess::open(sln_p, FileAccess::WRITE); if (f.is_null()) return ToolResult::err("FILE_ERROR", "Failed to write " + sln_f);
        f->store_string(sln_content); f.unref();
        notify_file_changed(csproj_p); notify_file_changed(sln_p);
        Dictionary d; d["success"] = true; d["project_name"] = pn; d["sdk_version"] = sv;
        d["guid"] = guid; d["csproj"] = csproj_f; d["sln"] = sln_f; d["enable_nativeaot"] = aot; d["project_root"] = proot;
        return ToolResult::ok(d);
    }
};

} // namespace godot_mcp
