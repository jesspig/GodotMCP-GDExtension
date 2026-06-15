
#pragma once

#include "built_in/cmd_utils.hpp"
#include "built_in/tool_base.hpp"
#include "server/registry/handler_registry.hpp"

namespace godot_mcp {

class FindToolTool : public ITool {
public:
    void set_registry(HandlerRegistry *reg) override { reg_ = reg; }

    String name() const override { return "find_tool"; }
    String category() const override { return "meta_tools"; }
    String brief() const override { return "Search for tools by name, keyword, or description"; }
    String description() const override {
        return "Search the tool registry for matching tools. Supports exact name match, "
               "prefix match, token search, and fulltext description search. "
               "Results are sorted by relevance (exact > prefix > token > fulltext) and usage frequency.";
    }
    Dictionary input_schema() const override {
        Dictionary schema;
        schema["type"] = "object";
        Dictionary props;
        Dictionary q;
        q["type"] = "string";
        q["description"] = "Search query - supports name, keyword, or partial text";
        props["query"] = q;
        Dictionary cat;
        cat["type"] = "string";
        cat["description"] = "Optional category filter (e.g. meta_tools, node_tools, editor_tools)";
        props["category"] = cat;
        Dictionary lim;
        lim["type"] = "integer";
        lim["description"] = "Maximum results to return (default 20)";
        props["limit"] = lim;
        schema["properties"] = props;
        Array req;
        req.push_back("query");
        schema["required"] = req;
        return schema;
    }
    bool is_meta() const override { return true; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        if (!reg_) {
            return ToolResult::err("NO_REGISTRY", "HandlerRegistry not available");
        }
        String query = ctx.args.get("query", "");
        String category = ctx.args.get("category", "");
        // args_int handles INT/FLOAT/BOOL and falls back to the default;
        // a raw Dictionary::get -> Variant -> int would abort on a non-numeric
        // payload (e.g. client sends "limit": "20").
        int limit = static_cast<int>(args_int(ctx.args, "limit", 20));
        if (query.is_empty()) {
            return ToolResult::err("MISSING_PARAM", "Missing required parameter: query");
        }
        Array results = reg_->search_tools(query, category, limit);
        Dictionary data;
        data["query"] = query;
        data["results"] = results;
        data["count"] = results.size();
        if (!category.is_empty()) data["category"] = category;
        return ToolResult::ok(data);
    }

private:
    HandlerRegistry *reg_ = nullptr;
};

} // namespace godot_mcp
