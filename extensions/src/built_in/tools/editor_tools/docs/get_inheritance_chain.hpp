
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/core/class_db.hpp>

namespace godot_mcp {

class GetInheritanceChainTool : public ITool {
public:
    String name() const noexcept override { return "get_inheritance_chain"; }
    String category() const noexcept override { return "editor_tools/docs"; }
    String brief() const noexcept override {
        return "Get the inheritance chain of a Godot class";
    }
    String description() const override {
        return "Walks the inheritance hierarchy of a Godot class from "
               "the class itself up to the root (Object or RefCounted). "
               "Returns the full chain as an ordered list.";
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

        Array chain;
        String current = class_name;
        while (!current.is_empty()) {
            Dictionary entry;
            entry["name"] = current;
            String parent = ClassDB::get_parent_class(current);
            entry["inherits"] = parent.is_empty() ? Variant() : Variant(parent);
            chain.append(entry);
            current = parent;
        }

        Dictionary data;
        data["class_name"] = class_name;
        data["chain"] = chain;
        data["depth"] = static_cast<int64_t>(chain.size());
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
