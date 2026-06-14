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

    rebuild_search_index();
    return true;
}

// ---------------------------------------------------------------------------
// ITool registration
// ---------------------------------------------------------------------------

void HandlerRegistry::register_tool(std::unique_ptr<ITool> tool, bool is_custom) {
    if (!tool) return;

    // 注入 registry 指针（meta 工具需要它回调查询）
    tool->set_registry(this);

    const String name = tool->name();

    // Populate ToolInfo for backward compatibility (category queries, schema, etc.)
    ToolInfo info;
    info.name = name;
    info.category = tool->category();
    info.brief = tool->brief();
    info.description = tool->description();
    info.category_description = tool->category_description();
    info.input_schema = tool->input_schema();
    info.is_meta = tool->is_meta();
    info.supports_undo = tool->supports_undo();
    info.is_destructive = tool->is_destructive();
    info.is_custom = is_custom;
    info.enabled = true;
    tool_info_[name] = info;

    itool_table_.emplace(name, std::move(tool));
}

void HandlerRegistry::finalize_registration() {
    rebuild_search_index();
}

// ---------------------------------------------------------------------------
// Unified execution: ITool dispatch
// ---------------------------------------------------------------------------

Dictionary HandlerRegistry::execute(const String &name, const Dictionary &args) {
    record_tool_call(name);

    // Find tool info for ability checks
    auto info_it = tool_info_.find(name);
    bool undoable = (info_it != tool_info_.end()) && info_it->value.supports_undo;

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

Dictionary HandlerRegistry::make_tool_entry(const ToolInfo &info) const {
    Dictionary d;
    d["name"] = info.name;
    d["brief"] = info.brief;
    d["description"] = info.description;

    // MCP 协议要求 inputSchema.type == "object"。
    // 兜底：若上游（builtin 工具 / SDK 自定义工具）未填 type，自动补全。
    Dictionary schema = info.input_schema;
    if (!schema.has("type")) {
        schema["type"] = "object";
        if (!schema.has("properties")) {
            schema["properties"] = Dictionary();
        }
    }
    d["inputSchema"] = schema;
    d["supports_undo"] = info.supports_undo;
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
    // Replace underscores with spaces, then capitalize first letter
    String result = seg.replace("_", " ");
    return result.substr(0, 1).to_upper() + result.substr(1);
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
    struct CatNode {
        String name;
        String description;
        int direct = 0;
        int total = 0;
        HashMap<String, CatNode> children;
    };

    CatNode root;

    for (const KeyValue<String, ToolInfo> &kv : tool_info_) {
        if (!kv.value.enabled) continue;
        const String &orig_cat = kv.value.category;
        if (orig_cat.is_empty()) continue;

        const PackedStringArray segments = orig_cat.split("/");
        CatNode *node = &root;
        for (int i = 0; i < segments.size(); i++) {
            const String seg = segments[i];
            if (!node->children.has(seg)) {
                CatNode child;
                child.name = seg;
                node->children[seg] = child;
            }
            node = &node->children[seg];
            node->total++;

            // 中间节点/叶子节点都尝试填 desc,仅用 category_description()
            // 不用 brief 兜底(brief 粒度太细,不适用于分类描述;
            // HashMap 顺序不可控,brief 兜底会导致子分类 desc 错位)
            if (node->description.is_empty() && !kv.value.category_description.is_empty()) {
                node->description = kv.value.category_description;
            }
        }
        // 叶子节点:直接挂载 +1
        node->direct++;
    }

    // 自动发现顶级分类的描述(description)
    // label 使用 prettify_segment() 自动美化(如 "editor_tools" → "Editor Tools")
    for (KeyValue<String, CatNode> &kv : root.children) {
        const String &first_seg = kv.key;
        for (const KeyValue<String, ToolInfo> &ti : tool_info_) {
            if (!ti.value.enabled) continue;
            int slash = ti.value.category.find("/");
            String ti_first = (slash >= 0) ? ti.value.category.substr(0, slash) : ti.value.category;
            if (ti_first != first_seg) continue;

            if (kv.value.description.is_empty() && !ti.value.category_description.is_empty()) {
                kv.value.description = ti.value.category_description;
            }
        }
    }

    // Recursive converter: CatNode → Array of Dictionaries, sorted by name
    std::function<Array(const CatNode &, const String &)> node_to_subs =
        [&](const CatNode &parent, const String &parent_path) -> Array {
            std::vector<Dictionary> entries;
            for (const KeyValue<String, CatNode> &kv : parent.children) {
                const CatNode &child = kv.value;
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

    return node_to_subs(root, String());
}

Array HandlerRegistry::get_tools_in_category(const String &category) const {
    Array result;
    for (const KeyValue<String, ToolInfo> &kv : tool_info_) {
        if (!kv.value.enabled) continue;
        if (kv.value.category == category) {
            result.push_back(make_tool_entry(kv.value));
        }
    }
    return result;
}

// ---------------------------------------------------------------------------
// Always-on tools (meta-tools + godot_info)
// ---------------------------------------------------------------------------

Array HandlerRegistry::get_always_on_tools() const {
    Array result;
    for (const KeyValue<String, ToolInfo> &kv : tool_info_) {
        if (!kv.value.enabled) continue;
        if (kv.value.is_meta) {
            result.push_back(make_tool_entry(kv.value));
        }
    }
    return result;
}

// ---------------------------------------------------------------------------
// Counts
// ---------------------------------------------------------------------------

int HandlerRegistry::builtin_tool_count() const {
    int count = 0;
    for (const KeyValue<String, ToolInfo> &kv : tool_info_) {
        if (!kv.value.is_custom) ++count;
    }
    return count;
}

int HandlerRegistry::custom_tool_count() const {
    int count = 0;
    for (const KeyValue<String, ToolInfo> &kv : tool_info_) {
        if (kv.value.is_custom) ++count;
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

void HandlerRegistry::rebuild_search_index() {
    search_index_.clear();
    for (const KeyValue<String, ToolInfo> &kv : tool_info_) {
        PackedStringArray tokens = tokenize(kv.value.name + String(" ") + kv.value.brief + String(" ") + kv.value.description);
        for (int i = 0; i < tokens.size(); ++i) {
            if (!search_index_.has(tokens[i])) {
                search_index_[tokens[i]] = Array();
            }
            search_index_[tokens[i]].push_back(kv.key);
        }
    }
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

    // Phase 1: Exact match (weight 1000)
    for (const KeyValue<String, ToolInfo> &kv : tool_info_) {
        if (!category.is_empty() && !kv.value.category.begins_with(category)) continue;
        if (kv.value.name.to_lower() == q) {
            best_weight[kv.key] = 1000;
        }
    }

    // Phase 2: Prefix match (weight 500)
    for (const KeyValue<String, ToolInfo> &kv : tool_info_) {
        if (!category.is_empty() && !kv.value.category.begins_with(category)) continue;
        if (kv.value.name.to_lower().begins_with(q)) {
            if (!best_weight.has(kv.key) || best_weight[kv.key] < 500) {
                best_weight[kv.key] = 500;
            }
        }
    }

    // Phase 3: Token fuzzy — any token contains query (weight 200)
    for (const KeyValue<String, ToolInfo> &kv : tool_info_) {
        if (!category.is_empty() && !kv.value.category.begins_with(category)) continue;
        PackedStringArray tokens = tokenize(kv.value.name + String(" ") + kv.value.brief + String(" ") + kv.value.description);
        for (int i = 0; i < tokens.size(); ++i) {
            if (tokens[i].find(q) >= 0) {
                if (!best_weight.has(kv.key) || best_weight[kv.key] < 200) {
                    best_weight[kv.key] = 200;
                }
                break;
            }
        }
    }

    // Phase 4: Fulltext description substring (weight 50)
    for (const KeyValue<String, ToolInfo> &kv : tool_info_) {
        if (!category.is_empty() && !kv.value.category.begins_with(category)) continue;
        if (kv.value.description.to_lower().find(q) >= 0) {
            if (!best_weight.has(kv.key) || best_weight[kv.key] < 50) {
                best_weight[kv.key] = 50;
            }
        }
    }

    // Convert to sortable vector
    struct ScoredTool {
        String name;
        int weight;
        int freq;
    };
    std::vector<ScoredTool> scored;
    for (const KeyValue<String, int> &kv : best_weight) {
        ScoredTool st;
        st.name = kv.key;
        st.weight = kv.value;
        auto fit = freq_index_.find(kv.key);
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
    for (int i = 0; i < (int)scored.size() && i < limit; ++i) {
        Dictionary entry;
        entry["name"] = scored[i].name;
        const ToolInfo *info = find_tool_info(scored[i].name);
        if (info) {
            entry["brief"] = info->brief;
            entry["category"] = info->category;
            entry["description"] = info->description;
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
    for (const KeyValue<String, ToolInfo> &kv : tool_info_) {
        if (kv.value.name.to_lower().begins_with(p)) {
            auto fit = freq_index_.find(kv.key);
            candidates[kv.key] = (fit != freq_index_.end()) ? fit->value : 0;
        }
    }

    // Phase 2: Token prefix match for tools not already matched by name
    for (const KeyValue<String, ToolInfo> &kv : tool_info_) {
        if (candidates.has(kv.key)) continue;
        PackedStringArray tokens = tokenize(kv.value.name + String(" ") + kv.value.brief + String(" ") + kv.value.description);
        for (int i = 0; i < tokens.size(); ++i) {
            if (tokens[i].begins_with(p)) {
                auto fit = freq_index_.find(kv.key);
                candidates[kv.key] = (fit != freq_index_.end()) ? fit->value : 0;
                break;
            }
        }
    }

    // Sort by freq desc
    struct SuggEntry {
        String name;
        int freq;
    };
    std::vector<SuggEntry> sorted;
    for (const KeyValue<String, int> &kv : candidates) {
        sorted.push_back({kv.key, kv.value});
    }
    std::sort(sorted.begin(), sorted.end(), [](const SuggEntry &a, const SuggEntry &b) {
        return a.freq > b.freq;
    });

    Array result;
    for (int i = 0; i < sorted.size() && i < limit; ++i) {
        result.push_back(sorted[i].name);
    }
    return result;
}

}  // namespace godot_mcp
