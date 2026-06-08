// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "server/registry/handler_registry.hpp"

namespace godot_mcp {

class GetCategoriesTool : public ITool {
public:
    void set_registry(HandlerRegistry *reg) override { reg_ = reg; }

    String name() const override { return "get_categories"; }
    String category() const override { return "meta_tools"; }
    String brief() const override { return String::utf8("以树型结构列出工具分类，支持路径钻取和深度控制"); }
    String description() const override {
        return String::utf8("返回工具分类树。可指定 path 钻取到特定分类，"
                            "max_depth 控制展开深度（默认 3，-1 为无限制）。");
    }
    Dictionary input_schema() const override {
        Dictionary schema;
        schema["type"] = "object";
        Dictionary props;

        Dictionary p;
        p["type"] = "string";
        p["description"] = String::utf8("分类路径，空则从根开始。如 node_tools/property/Node/CanvasItem");
        props["path"] = p;

        Dictionary d;
        d["type"] = "integer";
        d["description"] = String::utf8("最大深度（默认 3，-1 为无限制）");
        props["max_depth"] = d;

        schema["properties"] = props;
        return schema;
    }
    bool is_meta() const override { return true; }

protected:
    // 在 categories 数组中按 id 查找指定 segment
    // 返回该节点的副本，未找到则返回空 Dictionary
    static Dictionary find_by_id(const Array &categories, const String &id) {
        for (int i = 0; i < categories.size(); ++i) {
            Dictionary cat = categories[i];
            if (cat.get("id", "") == id) {
                return cat;
            }
        }
        return Dictionary();
    }

    // 递归裁剪：depth 从 1 开始计数，超过 max_depth 时移除 subcategories
    static Dictionary trim_depth(const Dictionary &node, int max_depth, int depth) {
        Dictionary out = node;
        if (max_depth >= 0 && depth >= max_depth) {
            out.erase("subcategories");
            return out;
        }
        if (out.has("subcategories")) {
            Array subs = out["subcategories"];
            Array trimmed;
            for (int i = 0; i < subs.size(); ++i) {
                trimmed.push_back(trim_depth(subs[i], max_depth, depth + 1));
            }
            out["subcategories"] = trimmed;
        }
        return out;
    }

    // 递归查找路径节点：返回找到的节点副本（已裁剪），空 Dictionary 表示未找到
    static Dictionary find_path_node(const Array &categories,
                                     const PackedStringArray &segments,
                                     int seg_idx,
                                     int max_depth, int depth) {
        if (seg_idx >= segments.size()) return Dictionary();

        Dictionary node = find_by_id(categories, segments[seg_idx]);
        if (node.is_empty()) return node;

        if (seg_idx == segments.size() - 1) {
            return trim_depth(node, max_depth, depth);
        }

        if (node.has("subcategories")) {
            return find_path_node(node["subcategories"], segments,
                                  seg_idx + 1, max_depth, depth);
        }
        return Dictionary();
    }

    Dictionary execute_impl(const ToolContext &ctx) override {
        if (!reg_) {
            return ToolResult::err("NO_REGISTRY", "HandlerRegistry not available");
        }

        const Array all = reg_->get_categories();
        const String path = ctx.args.get("path", "");
        const int max_depth = ctx.args.get("max_depth", 3);
        Array result;

        if (path.is_empty()) {
            for (int i = 0; i < all.size(); ++i) {
                result.push_back(trim_depth(all[i], max_depth, 1));
            }
        } else {
            const PackedStringArray segments = path.split("/");
            Dictionary found = find_path_node(all, segments, 0, max_depth, 1);
            if (found.is_empty()) {
                return ToolResult::err("CATEGORY_NOT_FOUND",
                    String::utf8("未找到分类路径: ") + path);
            }
            result.push_back(found);
        }

        Dictionary data;
        data["categories"] = result;
        return ToolResult::ok(data);
    }

private:
    HandlerRegistry *reg_ = nullptr;
};

} // namespace godot_mcp
