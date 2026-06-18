
#pragma once

#include "built_in/cmd_utils.hpp"
#include "built_in/cmd_utils/schema_builder.hpp"
#include "built_in/tool_base.hpp"
#include "server/registry/handler_registry.hpp"

namespace godot_mcp {

class FindToolTool : public ITool {
public:
    void set_registry(HandlerRegistry *reg) override { reg_ = reg; }

    String name() const noexcept override { return "find_tool"; }
    String category() const noexcept override { return "meta_tools"; }
    String brief() const noexcept override { return "Search for tools by name, keyword, or description"; }
    String description() const override {
        return "Search the tool registry for matching tools. Supports exact name match, "
               "prefix match, token search, and fulltext description search. "
               "Results are sorted by relevance (exact > prefix > token > fulltext) and usage frequency.";
    }
    Dictionary build_input_schema() const override {
        return SchemaBuilder()
            .prop("query", "string", "Search query - supports name, keyword, or partial text")
            .prop("category", "string", "Optional category filter (e.g. meta_tools, node_tools, editor_tools)")
            .prop("limit", "integer", "Maximum results to return (default 20)")
            .required({"query"})
            .build();
    }
    bool is_meta() const noexcept override { return true; }

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
