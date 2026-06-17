
#pragma once

#include "built_in/cmd_utils/schema_builder.hpp"
#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/core/class_db.hpp>

namespace godot_mcp {

class GetPropertyDocTool : public ITool {
public:
    String name() const noexcept override { return "get_property_doc"; }
    String category() const noexcept override { return "editor_tools/docs"; }
    String brief() const noexcept override {
        return "Get detailed documentation for a class property";
    }
    String description() const override {
        return "Returns detailed information about a specific property "
               "of a Godot class, including type, getter/setter, "
               "default value, and usage flags.";
    }
    Dictionary build_input_schema() const override {
        return SchemaBuilder()
            .prop("class_name", "string", "Godot class name")
            .prop("property", "string", "Property name to inspect")
            .required({"class_name", "property"})
            .build();
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String class_name = args_string(ctx.args, "class_name");
        String property = args_string(ctx.args, "property");
        if (class_name.is_empty() || property.is_empty()) {
            return ToolResult::err("BAD_PARAM", "class_name and property are required");
        }

        // Search in class property list
        godot::TypedArray<Dictionary> props = ClassDB::class_get_property_list(class_name, false);
        Dictionary found;
        for (int i = 0; i < props.size(); i++) {
            Dictionary p = props[i];
            if (String(p["name"]) == property) {
                found = p;
                break;
            }
        }
        if (found.is_empty()) {
            return ToolResult::err("NOT_FOUND",
                "Property '" + property + "' not found in class " + class_name);
        }

        Dictionary data;
        data["class_name"] = class_name;
        data["property"] = property;
        data["type"] = static_cast<int64_t>(found.get("type", 0));
        data["class_name_hint"] = found.get("class_name", "");
        data["hint"] = static_cast<int64_t>(found.get("hint", 0));
        data["hint_string"] = found.get("hint_string", "");
        data["usage"] = static_cast<int64_t>(found.get("usage", 0));
        data["getter"] = String(ClassDB::class_get_property_getter(class_name, property));
        data["setter"] = String(ClassDB::class_get_property_setter(class_name, property));

        Variant default_val = ClassDB::class_get_property_default_value(class_name, property);
        if (default_val.get_type() != Variant::NIL) {
            data["default_value"] = default_val;
        }

        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
