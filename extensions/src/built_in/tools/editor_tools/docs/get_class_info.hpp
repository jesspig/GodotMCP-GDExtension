
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/object.hpp>

namespace godot_mcp {

class GetClassInfoTool : public ITool {
public:
    String name() const override { return "get_class_info"; }
    String category() const override { return "editor_tools/docs"; }
    String brief() const override {
        return "Get information about a Godot class";
    }
    String description() const override {
        return "Instantiate a Godot class and return its hierarchy, "
               "methods, properties, and signals. "
               "Provides detailed class metadata for documentation.";
    }
    Dictionary build_input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Godot class name to inspect";
            props["class_name"] = p;
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
        if (class_name.is_empty()) {
            return ToolResult::err("BAD_PARAM", "class_name is required");
        }

        Object *obj = ClassDB::instantiate(class_name);
        if (!obj) {
            return ToolResult::err("BAD_CLASS",
                "Cannot instantiate class: " + class_name +
                ". It may be abstract or not registered.");
        }

        String actual_class = obj->get_class();
        String parent_class = ClassDB::get_parent_class(actual_class);
        String inherits = actual_class;

        // Walk the inheritance chain
        Array inheritance_chain;
        inheritance_chain.append(actual_class);
        while (!parent_class.is_empty()) {
            inheritance_chain.append(parent_class);
            inherits = parent_class;
            parent_class = ClassDB::get_parent_class(inherits);
        }

        Array method_list = obj->get_method_list();
        Array property_list = obj->get_property_list();
        Array signal_list = obj->get_signal_list();

        Array methods_json;
        for (int i = 0; i < method_list.size(); i++) {
            Dictionary m = method_list[i];
            Dictionary entry;
            entry["name"] = m.get("name", "");
            Variant ret = m.get("return", Variant());
            entry["return_type"] = ret.get_type() != Variant::NIL
                ? String(Dictionary(ret).get("type", "")) : String("void");
            entry["args"] = m.get("args", Array());
            entry["flags"] = static_cast<int64_t>(m.get("flags", 0));
            methods_json.append(entry);
        }

        Array properties_json;
        for (int i = 0; i < property_list.size(); i++) {
            Dictionary p = property_list[i];
            Dictionary entry;
            entry["name"] = p.get("name", "");
            entry["type"] = static_cast<int64_t>(p.get("type", 0));
            entry["class_name"] = p.get("class_name", "");
            entry["usage"] = static_cast<int64_t>(p.get("usage", 0));
            properties_json.append(entry);
        }

        Array signals_json;
        for (int i = 0; i < signal_list.size(); i++) {
            Dictionary s = signal_list[i];
            Dictionary entry;
            entry["name"] = s.get("name", "");
            entry["args"] = s.get("args", Array());
            signals_json.append(entry);
        }

        memdelete(obj);

        Dictionary data;
        data["class_name"] = actual_class;
        data["inherits"] = inherits;
        data["inheritance_chain"] = inheritance_chain;
        data["method_count"] = static_cast<int64_t>(methods_json.size());
        data["property_count"] = static_cast<int64_t>(properties_json.size());
        data["signal_count"] = static_cast<int64_t>(signals_json.size());
        data["methods"] = methods_json;
        data["properties"] = properties_json;
        data["signals"] = signals_json;
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
