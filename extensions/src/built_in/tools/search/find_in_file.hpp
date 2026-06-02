// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/tools/search/search_helpers.hpp"
#include <godot_cpp/classes/file_access.hpp>

namespace godot_mcp {

class FindInFileTool : public ITool {
public:
    String name() const override { return "find_in_file"; }
    String category() const override { return "search"; }
    String brief() const override { return "Find pattern in a single file"; }
    String category_description() const override { return String::utf8("文件内容的查找与替换"); }
    String description() const override { return "Find pattern in a single file"; }
    Dictionary input_schema() const override {
        Dictionary s; s["type"] = "object"; Dictionary p;
        p["path"] = [](){Dictionary d;d["type"]="string";d["description"]="File path";return d;}();
        p["pattern"] = [](){Dictionary d;d["type"]="string";d["description"]="Search pattern";return d;}();
        p["literal"] = [](){Dictionary d;d["type"]="boolean";d["description"]="Literal match (default: false)";return d;}();
        p["max_matches"] = [](){Dictionary d;d["type"]="integer";d["description"]="Max matches (default: 200)";return d;}();
        s["properties"] = p; Array r; r.push_back("path"); r.push_back("pattern"); s["required"] = r; return s;
    }
protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String path = args_string(ctx.args, "path"), pattern = args_string(ctx.args, "pattern");
        if (pattern.is_empty()) return ToolResult::err("MISSING_PARAM", "missing 'pattern'");
        bool lit = args_bool(ctx.args, "literal", false);
        int64_t max = args_int(ctx.args, "max_matches", 200);
        String src = read_file_text(path);
        if (src.is_empty() && !FileAccess::file_exists(path))
            return ToolResult::err("NOT_FOUND", "File not found: " + path);
        Array matches = match_lines(src, pattern, lit, max);
        Dictionary d; d["path"] = path; d["matches"] = matches; d["count"] = (int64_t)matches.size();
        return ToolResult::ok(d);
    }
};

} // namespace godot_mcp
