// built_in/cmd_meta_tools.cpp — Progressive disclosure meta-tools
//
// list_tool_categories  — list all tool categories with counts
// list_tools            — list tools in a category (name + brief)
// get_tool_schema       — get full schema for one tool
// call_tool             — invoke any registered tool by name (fallback)

#include "cmd_utils.hpp"
#include "server/registry/handler_registry.hpp"

#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/variant.hpp>

using namespace godot;

namespace godot_mcp {
namespace {

// ---------------------------------------------------------------------------
// list_tool_categories
// ---------------------------------------------------------------------------
Dictionary cmd_list_tool_categories(const Dictionary & /*args*/, HandlerRegistry *reg) {
    Array categories = reg->get_categories();
    Dictionary result;
    result["categories"] = categories;
    return result;
}

// ---------------------------------------------------------------------------
// list_tools
// ---------------------------------------------------------------------------
Dictionary cmd_list_tools(const Dictionary &args, HandlerRegistry *reg) {
    const String category = args.get("category", "");
    if (category.is_empty()) {
        Dictionary err;
        err["error"] = "Missing required parameter: category";
        return err;
    }

    const String filter = args.get("filter", "");
    Array tools = reg->get_tools_in_category(category);

    if (!filter.is_empty()) {
        Array filtered;
        for (int i = 0; i < tools.size(); ++i) {
            Dictionary t = tools[i];
            const String name = t.get("name", "");
            const String brief = t.get("brief", "");
            if (name.findn(filter) >= 0 || brief.findn(filter) >= 0) {
                filtered.push_back(t);
            }
        }
        tools = filtered;
    }

    Dictionary result;
    result["category"] = category;
    result["tools"] = tools;
    result["count"] = tools.size();
    return result;
}

// ---------------------------------------------------------------------------
// get_tool_schema
// ---------------------------------------------------------------------------
Dictionary cmd_get_tool_schema(const Dictionary &args, HandlerRegistry *reg) {
    const String tool_name = args.get("tool_name", "");
    if (tool_name.is_empty()) {
        Dictionary err;
        err["error"] = "Missing required parameter: tool_name";
        return err;
    }

    const ToolInfo *info = reg->get_tool_schema(tool_name);
    if (!info) {
        Dictionary err;
        err["error"] = String("Tool not found: ") + tool_name;
        return err;
    }

    Dictionary result;
    result["name"] = info->name;
    result["description"] = info->description;
    result["brief"] = info->brief;
    result["category"] = info->category;
    result["source"] = info->source;
    result["input_schema"] = info->input_schema;
    return result;
}

// ---------------------------------------------------------------------------
// call_tool (fallback)
// ---------------------------------------------------------------------------
Dictionary cmd_call_tool(const Dictionary &args, HandlerRegistry *reg) {
    const String tool_name = args.get("tool_name", "");
    if (tool_name.is_empty()) {
        Dictionary err;
        err["error"] = "Missing required parameter: tool_name";
        return err;
    }

    const CommandFn *fn = reg->find(tool_name);
    if (!fn) {
        Dictionary err;
        err["error"] = String("Tool not found: ") + tool_name;
        return err;
    }

    Dictionary tool_args;
    const Variant arguments = args.get("arguments", Variant());
    if (arguments.get_type() == Variant::DICTIONARY) {
        tool_args = Dictionary(arguments);
    }

    return (*fn)(tool_args);
}

} // namespace

// ---------------------------------------------------------------------------
// Registration — uses a closure pattern to pass HandlerRegistry* to handlers
// ---------------------------------------------------------------------------
void register_meta_tools(HandlerRegistry &reg) {
    HandlerRegistry *reg_ptr = &reg;

    reg.register_tool("list_tool_categories",
                      [reg_ptr](const Dictionary &args) {
                          return cmd_list_tool_categories(args, reg_ptr);
                      });

    reg.register_tool("list_tools",
                      [reg_ptr](const Dictionary &args) {
                          return cmd_list_tools(args, reg_ptr);
                      });

    reg.register_tool("get_tool_schema",
                      [reg_ptr](const Dictionary &args) {
                          return cmd_get_tool_schema(args, reg_ptr);
                      });

    reg.register_tool("call_tool",
                      [reg_ptr](const Dictionary &args) {
                          return cmd_call_tool(args, reg_ptr);
                      });
}

} // namespace godot_mcp
