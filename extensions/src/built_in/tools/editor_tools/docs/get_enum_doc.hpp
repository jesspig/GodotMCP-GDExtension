
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/core/class_db.hpp>

namespace godot_mcp {

class GetEnumDocTool : public ITool {
public:
    String name() const override { return "get_enum_doc"; }
    String category() const override { return "editor_tools/docs"; }
    String brief() const override {
        return "Get documentation for a class enum";
    }
    String description() const override {
        return "Returns detailed information about a specific enum "
               "of a Godot class, including all enum constants and "
               "their values. Supports both regular enums and bitfields.";
    }
    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Godot class name";
            props["class_name"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Enum name to inspect (e.g., AlignMode, CornerType). "
                               "If empty, returns all enums for the class.";
            props["enum_name"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        {
            Array req;
            req.append("class_name");
            s["required"] = req;
        }
        return s;
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String class_name = args_string(ctx.args, "class_name");
        String enum_name = args_string(ctx.args, "enum_name");
        if (class_name.is_empty()) {
            return ToolResult::err("BAD_PARAM", "class_name is required");
        }

        PackedStringArray enum_list = ClassDB::class_get_enum_list(class_name, false);

        if (enum_name.is_empty()) {
            // Return all enums
            Array result;
            for (int i = 0; i < enum_list.size(); i++) {
                String en = enum_list[i];
                Dictionary entry;
                entry["name"] = en;
                entry["is_bitfield"] = ClassDB::is_class_enum_bitfield(class_name, en, false);
                PackedStringArray constants = ClassDB::class_get_enum_constants(class_name, en, false);
                Array const_list;
                for (int j = 0; j < constants.size(); j++) {
                    Dictionary c;
                    c["name"] = constants[j];
                    c["value"] = static_cast<int64_t>(ClassDB::class_get_integer_constant(class_name, constants[j]));
                    const_list.append(c);
                }
                entry["constants"] = const_list;
                entry["constant_count"] = static_cast<int64_t>(const_list.size());
                result.append(entry);
            }
            Dictionary data;
            data["class_name"] = class_name;
            data["enums"] = result;
            data["count"] = static_cast<int64_t>(result.size());
            return ToolResult::ok(data);
        }

        // Specific enum
        if (!ClassDB::class_has_enum(class_name, enum_name, false)) {
            return ToolResult::err("NOT_FOUND",
                "Enum '" + enum_name + "' not found in class " + class_name);
        }

        PackedStringArray constants = ClassDB::class_get_enum_constants(class_name, enum_name, false);
        Array const_list;
        for (int i = 0; i < constants.size(); i++) {
            Dictionary c;
            c["name"] = constants[i];
            c["value"] = static_cast<int64_t>(ClassDB::class_get_integer_constant(class_name, constants[i]));
            const_list.append(c);
        }

        Dictionary data;
        data["class_name"] = class_name;
        data["enum_name"] = enum_name;
        data["is_bitfield"] = ClassDB::is_class_enum_bitfield(class_name, enum_name, false);
        data["constants"] = const_list;
        data["constant_count"] = static_cast<int64_t>(const_list.size());
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
