
#pragma once

#include "built_in/cmd_utils/schema_builder.hpp"
#include "built_in/tool_base.hpp"
#include "server/registry/handler_registry.hpp"

namespace godot_mcp {

class GetToolsTool : public ITool {
public:
    void set_registry(HandlerRegistry *reg) override { reg_ = reg; }

    String name() const noexcept override { return "get_tools"; }
    String category() const noexcept override { return "meta_tools"; }
    String brief() const noexcept override { return String("List all tools under a category path"); }
    String description() const override {
        return String("Returns a brief list (id, name, description) of all tools registered "
                      "under the given category path. Does not include tools from subcategories.");
    }
    Dictionary build_input_schema() const override {
        return SchemaBuilder()
            .prop("category", "string", "Category path, e.g. meta_tools, node_tools/property/CanvasItem")
            .required({"category"})
            .build();
    }
    bool is_meta() const noexcept override { return true; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        if (!reg_) {
            return ToolResult::err("NO_REGISTRY", "HandlerRegistry not available");
        }
        const String category = ctx.args.get("category", "");
        if (category.is_empty()) {
            return ToolResult::err("MISSING_PARAM", "Missing required parameter: category");
        }

        const Array entries = reg_->get_tools_in_category(category);
        Array tools;
        for (int i = 0; i < entries.size(); ++i) {
            Dictionary entry = entries[i];
            Dictionary t;
            t["id"] = entry.get("name", "");
            t["name"] = entry.get("brief", "");
            t["description"] = entry.get("description", "");
            tools.push_back(t);
        }

        Dictionary data;
        data["category"] = category;
        data["tools"] = tools;
        data["count"] = tools.size();
        return ToolResult::ok(data);
    }

private:
    HandlerRegistry *reg_ = nullptr;
};

} // namespace godot_mcp
