// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/tools/search/search_helpers.hpp"

namespace godot_mcp {

class SearchProjectTool : public ITool {
public:
    String name() const override { return "search_project"; }
    String category() const override { return "search"; }
    String brief() const override { return "Search project files for pattern"; }
    String description() const override { return "Search project files for pattern"; }
    Dictionary input_schema() const override {
        Dictionary s; s["type"] = "object"; Dictionary p;
        p["pattern"] = [](){Dictionary d;d["type"]="string";d["description"]="Search pattern";return d;}();
        p["literal"] = [](){Dictionary d;d["type"]="boolean";d["description"]="Literal match (default: false)";return d;}();
        p["extensions"] = [](){Dictionary d;d["type"]="array";d["description"]="File extensions (default: gd,cs,tscn,tres,godot)";return d;}();
        p["root"] = [](){Dictionary d;d["type"]="string";d["description"]="Root directory (default: res://)";return d;}();
        p["include_addons"] = [](){Dictionary d;d["type"]="boolean";d["description"]="Include addons/";return d;}();
        p["max_matches"] = [](){Dictionary d;d["type"]="integer";d["description"]="Max matches (default: 500)";return d;}();
        p["max_files"] = [](){Dictionary d;d["type"]="integer";d["description"]="Max files to scan (default: 5000)";return d;}();
        s["properties"] = p; Array r; r.push_back("pattern"); s["required"] = r; return s;
    }
protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String pattern = args_string(ctx.args, "pattern");
        if (pattern.is_empty()) return ToolResult::err("MISSING_PARAM", "missing 'pattern'");
        bool lit = args_bool(ctx.args, "literal", false);
        Array exts;
        if (ctx.args.has("extensions") && ctx.args["extensions"].get_type() == Variant::ARRAY) exts = ctx.args["extensions"];
        else { exts.append("gd"); exts.append("cs"); exts.append("tscn"); exts.append("tres"); exts.append("godot"); }
        String root = args_string(ctx.args, "root", "res://");
        bool inc = args_bool(ctx.args, "include_addons", false);
        int64_t max_m = args_int(ctx.args, "max_matches", 500), max_f = args_int(ctx.args, "max_files", 5000);
        Array files; walk_project_dir(root, exts, inc, max_f, files);
        Array all; int64_t scanned = 0;
        for (int i = 0; i < files.size() && all.size() < max_m; i++) {
            String src = read_file_text((String)files[i]);
            if (!src.is_empty()) {
                scanned++;
                Array hits = match_lines(src, pattern, lit, max_m - all.size());
                for (int j = 0; j < hits.size(); j++) { Dictionary h = hits[j]; h["path"] = files[i]; all.append(h); }
            }
        }
        Dictionary d; d["matches"] = all; d["count"] = (int64_t)all.size();
        d["files_scanned"] = scanned; d["truncated"] = all.size() >= max_m || files.size() >= max_f;
        return ToolResult::ok(d);
    }
};

} // namespace godot_mcp
