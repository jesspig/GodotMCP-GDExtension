// @tool register
// @source builtin
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "built_in/tools/search/search_helpers.hpp"
#include <godot_cpp/classes/file_access.hpp>

namespace godot_mcp {

class FindAndReplaceTool : public ITool {
public:
    String name() const override { return "find_and_replace"; }
    String category() const override { return "search"; }
    String brief() const override { return "Find and replace in a file"; }
    String description() const override { return "Find and replace in a file"; }
    Dictionary input_schema() const override {
        Dictionary s; s["type"] = "object"; Dictionary p;
        p["path"] = [](){Dictionary d;d["type"]="string";d["description"]="File path";return d;}();
        p["pattern"] = [](){Dictionary d;d["type"]="string";d["description"]="Search pattern";return d;}();
        p["replacement"] = [](){Dictionary d;d["type"]="string";d["description"]="Replacement text";return d;}();
        p["literal"] = [](){Dictionary d;d["type"]="boolean";d["description"]="Literal match (default: true)";return d;}();
        p["max_replacements"] = [](){Dictionary d;d["type"]="integer";d["description"]="Max (default: 10000)";return d;}();
        s["properties"] = p; Array r; r.push_back("path"); r.push_back("pattern"); r.push_back("replacement"); s["required"] = r; return s;
    }
protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String path = args_string(ctx.args, "path"), pattern = args_string(ctx.args, "pattern"), repl = args_string(ctx.args, "replacement");
        if (pattern.is_empty()) return ToolResult::err("MISSING_PARAM", "missing 'pattern'");
        bool lit = args_bool(ctx.args, "literal", true);
        String src = read_file_text(path);
        if (src.is_empty() && !FileAccess::file_exists(path))
            return ToolResult::err("NOT_FOUND", "File not found: " + path);
        int64_t count = 0;
        String result;
        if (lit) {
            result = src.replace(pattern, repl);
            int idx = 0; while ((idx = src.find(pattern, idx)) >= 0) { count++; idx += pattern.length(); }
        } else {
            return ToolResult::err("UNSUPPORTED", "Regex replace not supported yet; use literal=true");
        }
        if (count == 0) { Dictionary d; d["path"] = path; d["replacements"] = (int64_t)0; d["hint"] = "No matches found"; return ToolResult::ok(d); }
        Ref<FileAccess> f = FileAccess::open(path, FileAccess::WRITE);
        if (f.is_null()) return ToolResult::err("FILE_ERROR", "Failed to open '" + path + "' for writing");
        f->store_string(result);
        notify_file_changed(path);
        Dictionary d; d["path"] = path; d["replacements"] = count;
        d["truncated"] = count >= args_int(ctx.args, "max_replacements", 10000);
        return ToolResult::ok(d);
    }
};

} // namespace godot_mcp
