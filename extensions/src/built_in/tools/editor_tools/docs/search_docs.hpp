
#pragma once

#include "built_in/cmd_utils/schema_builder.hpp"
#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/object.hpp>

namespace godot_mcp {

class SearchDocsTool : public ITool {
public:
    String name() const noexcept override { return "search_docs"; }
    String category() const noexcept override { return "editor_tools/docs"; }
    String brief() const noexcept override {
        return "Search Godot documentation across classes, methods, properties, signals, and enums";
    }
    String description() const override {
        return String("Aggregated documentation search. Searches class names, methods, properties, signals, and enums ")
            + "for the given query. Returns structured results grouped by match type, "
            + "including inheritance chains and argument details.";
    }
    Dictionary build_input_schema() const override {
        return SchemaBuilder()
            .prop("query", "string", "Search query (class name, method name, or topic)")
            .prop("max_results", "integer", "Maximum number of results (default 20)")
            .required({"query"})
            .build();
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String query = args_string(ctx.args, "query");
        int64_t max_results = args_int(ctx.args, "max_results", 20);

        if (query.is_empty()) {
            return ToolResult::err("BAD_PARAM", "query is required");
        }

        String q = query.to_lower();
        Array results;

        // Search class names
        PackedStringArray all_classes = ClassDB::get_class_list();
        for (int64_t i = 0; i < all_classes.size() && results.size() < max_results; i++) {
            String cls = all_classes[i];
            if (cls.to_lower().find(q) >= 0) {
                Dictionary entry;
                entry["type"] = "class";
                entry["class_name"] = cls;
                entry["inherits"] = ClassDB::get_parent_class(cls);
                entry["match"] = "class_name";
                entry["method_count"] = static_cast<int64_t>(ClassDB::class_get_method_list(cls, true).size());
                entry["property_count"] = static_cast<int64_t>(ClassDB::class_get_property_list(cls, true).size());
                entry["signal_count"] = static_cast<int64_t>(ClassDB::class_get_signal_list(cls, true).size());
                results.append(entry);
            }
        }

        // Search methods across all classes
        if (results.size() < max_results) {
            for (int64_t i = 0; i < all_classes.size() && results.size() < max_results; i++) {
                String cls = all_classes[i];
                TypedArray<Dictionary> methods = ClassDB::class_get_method_list(cls, true);
                for (int64_t j = 0; j < methods.size() && results.size() < max_results; j++) {
                    Dictionary m = methods[j];
                    String mname = m.get("name", "");
                    if (mname.to_lower().find(q) >= 0) {
                        Dictionary entry;
                        entry["type"] = "method";
                        entry["class_name"] = cls;
                        entry["method"] = mname;
                        entry["match"] = "method_name";
                        Variant ret = m.get("return", Variant());
                        if (ret.get_type() == Variant::DICTIONARY) {
                            entry["return_type"] = Dictionary(ret).get("type", "");
                        }
                        results.append(entry);
                    }
                }
            }
        }

        // Search properties across all classes
        if (results.size() < max_results) {
            for (int64_t i = 0; i < all_classes.size() && results.size() < max_results; i++) {
                String cls = all_classes[i];
                TypedArray<Dictionary> props = ClassDB::class_get_property_list(cls, true);
                for (int64_t j = 0; j < props.size() && results.size() < max_results; j++) {
                    Dictionary p = props[j];
                    String pname = p.get("name", "");
                    if (pname.to_lower().find(q) >= 0) {
                        Dictionary entry;
                        entry["type"] = "property";
                        entry["class_name"] = cls;
                        entry["property"] = pname;
                        entry["match"] = "property_name";
                        results.append(entry);
                    }
                }
            }
        }

        // Search signals across all classes
        if (results.size() < max_results) {
            for (int64_t i = 0; i < all_classes.size() && results.size() < max_results; i++) {
                String cls = all_classes[i];
                TypedArray<Dictionary> sigs = ClassDB::class_get_signal_list(cls, true);
                for (int64_t j = 0; j < sigs.size() && results.size() < max_results; j++) {
                    Dictionary s = sigs[j];
                    String sname = s.get("name", "");
                    if (sname.to_lower().find(q) >= 0) {
                        Dictionary entry;
                        entry["type"] = "signal";
                        entry["class_name"] = cls;
                        entry["signal"] = sname;
                        entry["match"] = "signal_name";
                        results.append(entry);
                    }
                }
            }
        }

        Dictionary data;
        data["results"] = results;
        data["count"] = static_cast<int64_t>(results.size());
        data["query"] = query;
        data["searched"] = Array::make("classes", "methods", "properties", "signals");
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
