// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>

namespace godot_mcp {

class CSharpBuildTool : public ITool {
public:
    String name() const override { return "csharp_build"; }
    String category() const override { return "script/csharp"; }
    String brief() const override { return "Build the C# project via dotnet"; }
    String description() const override { return "Runs 'dotnet build' on the project root."; }
    Dictionary input_schema() const override {
        Dictionary s; s["type"] = "object"; Dictionary p;
        p["configuration"] = [](){Dictionary d;d["type"]="string";d["description"]="Build config (default: Debug)";return d;}();
        s["properties"] = p; return s;
    }
protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String cfg = args_string(ctx.args, "configuration", "Debug");
        String proot = globalize_path("res://");
        if (proot.is_empty()) return ToolResult::err("PATH_ERROR", "Cannot resolve project root");
        PackedStringArray pargs; pargs.append("build"); pargs.append("--nologo"); pargs.append("--configuration"); pargs.append(cfg);
        Array output;
        int32_t exit_code = OS::get_singleton()->execute("dotnet", pargs, output, true);
        if (exit_code == 0) {
            Dictionary d;
            d["project_root"] = proot; d["configuration"] = cfg;
            d["exit_code"] = (int64_t)0; d["stdout"] = output.size() > 0 ? (String)output[0] : String("");
            return d;
        }
        return ToolResult::err("BUILD_FAILED", "dotnet build failed with code " + String::num_int64(exit_code));
    }
};

} // namespace godot_mcp
