
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/core/class_db.hpp>

namespace godot_mcp {

class GetMethodDocTool : public ITool {
public:
    String name() const override { return "get_method_doc"; }
    String category() const override { return "editor_tools/docs"; }
    String brief() const override {
        return "Get detailed documentation for a class method";
    }
    String description() const override {
        return "Returns detailed information about a specific method "
               "of a Godot class, including arguments, return type, "
               "and flags.";
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
            p["description"] = "Method name to inspect";
            props["method"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        {
            Array req;
            req.append("class_name");
            req.append("method");
            s["required"] = req;
        }
        return s;
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String class_name = args_string(ctx.args, "class_name");
        String method = args_string(ctx.args, "method");
        if (class_name.is_empty() || method.is_empty()) {
            return ToolResult::err("BAD_PARAM", "class_name and method are required");
        }

        if (!ClassDB::class_has_method(class_name, method, false)) {
            return ToolResult::err("NOT_FOUND",
                "Method '" + method + "' not found in class " + class_name);
        }

        TypedArray<Dictionary> methods = ClassDB::class_get_method_list(class_name, false);
        Dictionary found;
        for (int i = 0; i < methods.size(); i++) {
            Dictionary m = methods[i];
            if (String(m["name"]) == method) {
                found = m;
                break;
            }
        }

        Dictionary data;
        data["class_name"] = class_name;
        data["method"] = method;
        data["arg_count"] = (int64_t)ClassDB::class_get_method_argument_count(class_name, method, false);

        if (!found.is_empty()) {
            data["flags"] = (int64_t)found.get("flags", 0);
            Variant ret = found.get("return", Variant());
            if (ret.get_type() == Variant::DICTIONARY) {
                Dictionary ret_dict = ret;
                data["return_type"] = ret_dict.get("type", "");
                data["return_class_name"] = ret_dict.get("class_name", "");
            }
            Array args = found.get("args", Array());
            Array arg_info;
            for (int i = 0; i < args.size(); i++) {
                Dictionary a = args[i];
                Dictionary entry;
                entry["name"] = a.get("name", "");
                entry["type"] = a.get("type", "");
                entry["class_name"] = a.get("class_name", "");
                entry["has_default_value"] = a.has("default_value");
                if (a.has("default_value")) {
                    entry["default_value"] = a["default_value"];
                }
                arg_info.append(entry);
            }
            data["args"] = arg_info;
        }

        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
