
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/editor_interface.hpp>

namespace godot_mcp {

class SearchDocsTool : public ITool {
public:
    String name() const override { return "search_docs"; }
    String category() const override { return "editor_tools/docs"; }
    String brief() const override {
        return "Search Godot documentation";
    }
    String description() const override {
        return "Search the Godot documentation via the editor's help system. "
               "Supports class, method, and topic queries.";
    }
    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Search query (class name, method, or topic)";
            props["query"] = p;
        }
        {
            Dictionary p;
            p["type"] = "integer";
            p["description"] = "Maximum number of results (default 10)";
            props["max_results"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        {
            Array req;
            req.append("query");
            s["required"] = req;
        }
        return s;
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String query = args_string(ctx.args, "query");
        int64_t max_results = args_int(ctx.args, "max_results", 10);

        if (query.is_empty()) {
            return ToolResult::err("BAD_PARAM", "query is required");
        }

        godot::EditorInterface *ei = godot::EditorInterface::get_singleton();
        if (!ei) {
            return ToolResult::err("NO_EDITOR", "EditorInterface not available");
        }

        Array results;

        // Try editor help search
        Variant search_result = ei->call("search_help", query);
        if (search_result.get_type() == Variant::DICTIONARY) {
            Dictionary sr = search_result;
            Array items = sr.get("results", Array());
            for (int i = 0; i < items.size() && static_cast<int64_t>(results.size()) < max_results; i++) {
                Dictionary item = items[i];
                Dictionary entry;
                entry["class_name"] = item.get("class_name", "");
                entry["method"] = item.get("name", "");
                entry["description"] = item.get("description", "");
                results.append(entry);
            }
        }

        // Fallback: try to check if query matches a known class
        if (results.size() == 0) {
            // Try instantiating as a class to get basic info
            Object *obj = ClassDB::instantiate(query);
            if (obj) {
                Dictionary entry;
                entry["class_name"] = obj->get_class();
                entry["method"] = "";
                entry["description"] = "Class: " + obj->get_class() +
                    ", inherits: " + ClassDB::get_parent_class(obj->get_class());
                results.append(entry);
                memdelete(obj);
            }
        }

        Dictionary data;
        data["results"] = results;
        data["count"] = static_cast<int64_t>(results.size());
        data["query"] = query;
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
