#include "handler_registry.hpp"
#include "built_in/cmd_utils.hpp"
#include "built_in/tool_adapter.hpp"
#include "built_in/tool_base.hpp"

#include <algorithm>
#include <vector>

using namespace godot;

namespace godot_mcp {

HandlerRegistry::HandlerRegistry() = default;
HandlerRegistry::~HandlerRegistry() = default;

// ---------------------------------------------------------------------------
// Tool registration
// ---------------------------------------------------------------------------

bool HandlerRegistry::unregister_custom_tool(const String &name) {
    auto it_info = tool_info_.find(name);
    if (it_info == tool_info_.end() || !it_info->value.is_custom) {
        return false;
    }
    tool_info_.erase(name);

    auto it_tool = itool_table_.find(name);
    if (it_tool != itool_table_.end()) {
        itool_table_.erase(it_tool);
    }

    search_index_.erase(name);
    categories_dirty_ = true;
    return true;
}

// ---------------------------------------------------------------------------
// ITool registration
// ---------------------------------------------------------------------------

void HandlerRegistry::register_tool(std::unique_ptr<ITool> tool, bool is_custom) {
    if (!tool) return;

    tool->set_registry(this);

    const String name = tool->name();

    ToolInfo info;
    info.is_destructive = tool->is_destructive();
    info.is_custom = is_custom;
    info.enabled = true;
    info.tool_ptr = tool.get();
    tool_info_[name] = info;

    // 预构建搜索索引，避免 search_tools() 实时 tokenize
    {
        SearchIndexEntry idx_entry;
        const String search_text = name + String(" ") + tool->brief() + String(" ") + tool->description();
        idx_entry.tokens = tokenize(search_text);
        search_index_[name] = idx_entry;
    }

    itool_table_[name] = std::move(tool);
}

// ---------------------------------------------------------------------------
// Unified execution: ITool dispatch
// ---------------------------------------------------------------------------

Dictionary HandlerRegistry::execute(const String &name, const Dictionary &args) {
    record_tool_call(name);

    auto info_it = tool_info_.find(name);
    bool undoable = false;
    if (info_it != tool_info_.end()) {
        const ITool *tp = find_itool(name);
        if (tp) undoable = tp->supports_undo();
    }

    // Check ITool table first
    auto it = itool_table_.find(name);
    if (it != itool_table_.end()) {
        // Auto-Undo wrapping
        if (undoable) {
            EditorUndoRedoManager *undo_redo = get_undo_redo();
            if (undo_redo) {
                undo_redo->create_action(
                    String("MCP: ") + name,
                    UndoRedo::MERGE_ENDS);

                Dictionary result = it->second->execute(args);

                if (result.has("success") && result["success"].operator bool()) {
                    undo_redo->commit_action(false);
                } else {
                    undo_redo->commit_action(true);
                }

                return result;
            }
        }

        // No undo wrapping (tool doesn't support undo, or no undo manager)
        return it->second->execute(args);
    }

    Dictionary error;
    error["error"] = String("Tool not found: ") + name;
    return error;
}

// ---------------------------------------------------------------------------
// Lookup
// ---------------------------------------------------------------------------

const ToolInfo *HandlerRegistry::find_tool_info(const String &name) const {
    auto it = tool_info_.find(name);
    if (it == tool_info_.end()) return nullptr;
    return &it->value;
}

Dictionary HandlerRegistry::make_tool_entry(const String &name, const ToolInfo &info) const {
    Dictionary d;
    d["name"] = name;
    const ITool *tool = find_itool(name);
    if (!tool) return d;
    d["brief"] = tool->brief();
    d["description"] = tool->description();

    Dictionary schema = tool->input_schema();
    if (!schema.has("type")) {
        schema["type"] = "object";
        if (!schema.has("properties")) {
            schema["properties"] = Dictionary();
        }
    }
    d["inputSchema"] = schema;
    d["supports_undo"] = tool->supports_undo();
    d["is_destructive"] = info.is_destructive;
    return d;
}

// ---------------------------------------------------------------------------
// Category queries (for progressive disclosure)
// ---------------------------------------------------------------------------
//
// get_categories() 输出字段契约(每个分类节点):
//   id          : 分类段名(如 "node"、"property"),用于 get_tools 查询
//   name        : 分类展示名(如 "Node"、"Property")(prettify 自动美化,客户端 UI 显示用)
//   path        : 完整分类路径(如 "node/property"),用于 get_tools 查询
//   description : 分类描述(来自工具的 category_description(),自动发现)
//   tool_count  : 直接挂载到该分类的工具数(不含子分类)
//   total       : 包含子分类的累加值
//   subcategories : 子分类列表(数组),递归结构一致,若无则省略
namespace {

String prettify_segment(const String &seg) {
    if (seg.is_empty()) return seg;
    String result = seg.replace("_", " ");
    // 查找首字母位置（跳过开头的数字，如 "3d" → "3D", "2d" → "2D"）
    int first_letter = -1;
    for (int i = 0; i < result.length(); i++) {
        char32_t c = result[i];
        if ((c >= U'a' && c <= U'z') || (c >= U'A' && c <= U'Z')) {
            first_letter = i;
            break;
        }
    }
    if (first_letter >= 0) {
        String upper = result.substr(first_letter, 1).to_upper();
        result = result.substr(0, first_letter) + upper + result.substr(first_letter + 1);
    }
    return result;
}

}  // namespace

// 把 CatNode 序列化为 Dictionary(MCP 协议输出契约)。
//   id          : 分类段名(如 "node"、"property"),用于 get_tools 查询
//   name        : 分类展示名(如 "Node"、"Property"),客户端 UI 显示用
//   path        : 完整分类路径(如 "node/property"),用于 get_tools 查询
//   description : 分类描述(来自工具的 category_description(),自动发现)
//   tool_count  : 直接挂载到该分类的工具数(不含子分类)
//   total       : 包含子分类的累加值
//   subcategories : 子分类列表(数组),递归结构一致,若无则省略
static Dictionary cat_node_to_dict(const String &id, const String &name,
                                   const String &path,
                                   int direct, int total,
                                   const String &description,
                                   const Array &subs) {
    Dictionary d;
    d["id"] = id;
    d["name"] = name.is_empty() ? id : name;
    d["path"] = path;
    d["tool_count"] = direct;
    d["total"] = total;
    if (!description.is_empty())
        d["description"] = description;
    if (!subs.is_empty())
        d["subcategories"] = subs;
    return d;
}

Array HandlerRegistry::get_categories() const {
    if (!categories_dirty_) {
        return categories_cache_;
    }

    struct CatNode {
        String name;
        String description;
        int direct = 0;
        int total = 0;
        HashMap<String, CatNode> children;
    };

    CatNode root;
    // Track top-level category descriptions during single traversal
    HashMap<String, bool> top_level_filled;

    for (const auto &[name, info] : tool_info_) {
        if (!info.enabled) continue;
        const ITool *tool = find_itool(name);
        if (!tool) continue;
        const String orig_cat = tool->category();
        if (orig_cat.is_empty()) continue;

        const PackedStringArray segments = orig_cat.split("/");
        CatNode *node = &root;
        for (int64_t i = 0; i < segments.size(); i++) {
            const String seg = segments[i];
            if (!node->children.has(seg)) {
                CatNode child;
                child.name = seg;
                node->children[seg] = child;
            }
            node = &node->children[seg];
            node->total++;

            if (!node->description.is_empty()) continue;
            const String cat_desc = tool->category_description();
            if (cat_desc.is_empty()) continue;

            node->description = cat_desc;
            if (i == 0) {
                top_level_filled[seg] = true;
            }
        }
        node->direct++;
    }

    for (auto &[seg, cat_node] : root.children) {
        if (top_level_filled.has(seg)) continue;
        for (const auto &[tool_name, tool_info] : tool_info_) {
            if (!tool_info.enabled) continue;
            const ITool *tool = find_itool(tool_name);
            if (!tool) continue;
            const String tool_cat = tool->category();
            int slash = static_cast<int>(tool_cat.find("/"));
            String ti_first = (slash >= 0) ? tool_cat.substr(0, slash) : tool_cat;
            if (ti_first != seg) continue;
            const String cat_desc = tool->category_description();
            if (cat_node.description.is_empty() && !cat_desc.is_empty()) {
                cat_node.description = cat_desc;
            }
        }
    }

    // Recursive converter: CatNode → Array of Dictionaries, sorted by name
    std::function<Array(const CatNode &, const String &)> node_to_subs =
        [&](const CatNode &parent, const String &parent_path) -> Array {
            std::vector<Dictionary> entries;
            entries.reserve(parent.children.size());
            for (const auto &[_, child] : parent.children) {
                const String child_path = parent_path.is_empty()
                                              ? child.name
                                              : parent_path + String("/") + child.name;
                const Array subs = child.children.is_empty()
                                       ? Array()
                                       : node_to_subs(child, child_path);
                const String name = prettify_segment(child.name);
                entries.push_back(cat_node_to_dict(
                    child.name, name, child_path,
                    child.direct, child.total,
                    child.description, subs));
            }
            std::sort(entries.begin(), entries.end(),
                      [](const Dictionary &a, const Dictionary &b) {
                          return String(a["name"]) < String(b["name"]);
                      });
            Array sorted;
            for (const Dictionary &d : entries)
                sorted.push_back(d);
            return sorted;
        };

    Array result = node_to_subs(root, String());
    categories_cache_ = result;
    categories_dirty_ = false;
    return result;
}

Array HandlerRegistry::get_tools_in_category(const String &category) const {
    Array result;
    for (const auto &[name, info] : tool_info_) {
        if (!info.enabled) continue;
        const ITool *tool = find_itool(name);
        if (tool && tool->category() == category) {
            result.push_back(make_tool_entry(name, info));
        }
    }
    return result;
}

// ---------------------------------------------------------------------------
// Always-on tools (meta-tools + godot_info)
// ---------------------------------------------------------------------------

Array HandlerRegistry::get_always_on_tools() const {
    Array result;
    for (const auto &[name, info] : tool_info_) {
        if (!info.enabled) continue;
        const ITool *tool = find_itool(name);
        if (tool && tool->is_meta()) {
            result.push_back(make_tool_entry(name, info));
        }
    }
    return result;
}

// ---------------------------------------------------------------------------
// Counts
// ---------------------------------------------------------------------------

int HandlerRegistry::builtin_tool_count() const {
    int count = 0;
    for (const auto &[name, info] : tool_info_) {
        if (!info.is_custom) ++count;
    }
    return count;
}

int HandlerRegistry::custom_tool_count() const {
    int count = 0;
    for (const auto &[name, info] : tool_info_) {
        if (info.is_custom) ++count;
    }
    return count;
}

// ---------------------------------------------------------------------------
// Search engine
// ---------------------------------------------------------------------------

PackedStringArray HandlerRegistry::tokenize(const String &text) {
    String clean = text.to_lower();
    clean = clean.replace("_", " ").replace("/", " ").replace(".", " ").replace("-", " ");
    PackedStringArray raw = clean.split(" ", false);
    PackedStringArray result;
    for (int i = 0; i < raw.size(); ++i) {
        String t = raw[i].strip_edges();
        if (!t.is_empty()) {
            result.push_back(t);
        }
    }
    return result;
}

void HandlerRegistry::record_tool_call(const String &name) {
    auto it = freq_index_.find(name);
    if (it != freq_index_.end()) {
        it->value++;
    } else {
        freq_index_[name] = 1;
    }
}

Array HandlerRegistry::search_tools(const String &query, const String &category, int limit) const {
    if (query.is_empty()) return Array();

    const String q = query.to_lower().strip_edges();
    HashMap<String, int> best_weight;

    // Single pass: evaluate all matching strategies for each tool (权重降序检查)
    for (const auto &[tool_name, info] : tool_info_) {
        const ITool *tool = find_itool(tool_name);
        if (!tool) continue;
        if (!category.is_empty() && !tool->category().begins_with(category)) continue;

        const String name_lower = tool_name.to_lower();

        // Exact match (weight 1000)
        if (name_lower == q) {
            best_weight[tool_name] = 1000;
            continue;
        }

        // Prefix match (weight 500)
        if (name_lower.begins_with(q)) {
            best_weight[tool_name] = 500;
            continue;
        }

        // Token fuzzy — any token contains query (weight 200)
        bool token_matched = false;
        auto idx_it = search_index_.find(tool_name);
        if (idx_it != search_index_.end()) {
            const PackedStringArray &tokens = idx_it->value.tokens;
            for (int i = 0; i < tokens.size(); ++i) {
                if (tokens[i].find(q) >= 0) {
                    best_weight[tool_name] = 200;
                    token_matched = true;
                    break;
                }
            }
        }
        if (token_matched) continue;

        if (tool->description().to_lower().find(q) >= 0) {
            best_weight[tool_name] = 50;
        }
    }

    // Convert to sortable vector
    struct ScoredTool {
        String name;
        int weight;
        int freq;
    };
    std::vector<ScoredTool> scored;
    scored.reserve(best_weight.size());
    for (const auto &[tool_name, weight] : best_weight) {
        ScoredTool st;
        st.name = tool_name;
        st.weight = weight;
        auto fit = freq_index_.find(tool_name);
        st.freq = (fit != freq_index_.end()) ? fit->value : 0;
        scored.push_back(st);
    }

    // Sort: freq desc, then weight desc
    std::sort(scored.begin(), scored.end(), [](const ScoredTool &a, const ScoredTool &b) {
        if (a.freq != b.freq) return a.freq > b.freq;
        return a.weight > b.weight;
    });

    // Build output
    Array result;
    for (int i = 0; i < static_cast<int>(scored.size()) && i < limit; ++i) {
        Dictionary entry;
        entry["name"] = scored[i].name;
        const ITool *tool = find_itool(scored[i].name);
        if (tool) {
            entry["brief"] = tool->brief();
            entry["category"] = tool->category();
            entry["description"] = tool->description();
        }
        entry["frequency"] = scored[i].freq;
        result.push_back(entry);
    }

    return result;
}

Array HandlerRegistry::get_search_suggestions(const String &prefix, int limit) const {
    if (prefix.is_empty()) return Array();

    const String p = prefix.to_lower().strip_edges();
    HashMap<String, int> candidates;

    // Phase 1: Prefix match on tool names
    for (const auto &[tool_name, info] : tool_info_) {
        if (tool_name.to_lower().begins_with(p)) {
            auto fit = freq_index_.find(tool_name);
            candidates[tool_name] = (fit != freq_index_.end()) ? fit->value : 0;
        }
    }

    // Phase 2: Token prefix match for tools not already matched by name
    for (const auto &[tool_name, info] : tool_info_) {
        if (candidates.has(tool_name)) continue;
        auto idx_it = search_index_.find(tool_name);
        if (idx_it != search_index_.end()) {
            const PackedStringArray &tokens = idx_it->value.tokens;
            for (int i = 0; i < tokens.size(); ++i) {
                if (tokens[i].begins_with(p)) {
                    auto fit = freq_index_.find(tool_name);
                    candidates[tool_name] = (fit != freq_index_.end()) ? fit->value : 0;
                    break;
                }
            }
        }
    }

    // Sort by freq desc
    struct SuggEntry {
        String name;
        int freq;
    };
    std::vector<SuggEntry> sorted;
    sorted.reserve(candidates.size());
    for (const auto &[tool_name, freq] : candidates) {
        sorted.push_back({tool_name, freq});
    }
    std::sort(sorted.begin(), sorted.end(), [](const SuggEntry &a, const SuggEntry &b) {
        return a.freq > b.freq;
    });

    Array result;
    for (size_t i = 0; i < sorted.size() && static_cast<int>(i) < limit; ++i) {
        result.push_back(sorted[i].name);
    }
    return result;
}

}  // namespace godot_mcp
