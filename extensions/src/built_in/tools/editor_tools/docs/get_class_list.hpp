
#pragma once

#include "built_in/cmd_utils/schema_builder.hpp"
#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/core/class_db.hpp>

namespace godot_mcp {

class GetClassListTool : public ITool {
public:
    String name() const noexcept override { return "get_class_list"; }
    String category() const noexcept override { return "editor_tools/docs"; }
    String brief() const noexcept override {
        return "List all registered Godot classes";
    }
    String description() const override {
        return "Returns a list of all Godot classes registered in ClassDB. "
               "Can optionally filter by a parent class to show only subclasses.";
    }
    Dictionary build_input_schema() const override {
        return SchemaBuilder()
            .prop("inherit", "string", "Optional parent class name to filter (e.g., Node, Control, Resource)")
            .build();
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String inherit_filter = args_string(ctx.args, "inherit");

        PackedStringArray all_classes = ClassDB::get_class_list();
        Array result;

        for (int i = 0; i < all_classes.size(); i++) {
            String class_name = all_classes[i];
            if (!inherit_filter.is_empty()) {
                if (!ClassDB::is_parent_class(class_name, inherit_filter)) {
                    continue;
                }
            }
            Dictionary entry;
            entry["name"] = class_name;
            entry["inherits"] = String(ClassDB::get_parent_class(class_name));
            result.append(entry);
        }

        Dictionary data;
        data["classes"] = result;
        data["count"] = static_cast<int64_t>(result.size());
        data["filter"] = inherit_filter.is_empty() ? Variant() : Variant(inherit_filter);
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
