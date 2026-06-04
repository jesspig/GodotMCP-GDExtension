#include "handler_registry.hpp"
#include "built_in/tool_base.hpp"

#include <algorithm>
#include <vector>

using namespace godot;

namespace godot_mcp {

HandlerRegistry::HandlerRegistry() = default;

// ---------------------------------------------------------------------------
// Tool registration
// ---------------------------------------------------------------------------

void HandlerRegistry::register_custom_tool(const String &name, const String &category,
                                           const String &brief, const String &description,
                                           const Dictionary &schema, CommandFn fn,
                                           bool is_meta) {
    table_[name] = std::move(fn);

    ToolInfo info;
    info.name = name;
    info.category = category;
    info.brief = brief;
    info.description = description;
    info.category_description = String();   // SDK 工具无 category_description() 虚函数,显式留空
    info.input_schema = schema;
    info.is_meta = is_meta;
    info.is_custom = true;
    info.enabled = true;
    tool_info_[name] = info;
}

bool HandlerRegistry::unregister_custom_tool(const String &name) {
    auto it_info = tool_info_.find(name);
    if (it_info == tool_info_.end() || !it_info->value.is_custom) {
        return false;
    }
    tool_info_.erase(name);
    table_.erase(name);
    return true;
}

// ---------------------------------------------------------------------------
// ITool registration
// ---------------------------------------------------------------------------

void HandlerRegistry::register_tool(std::unique_ptr<ITool> tool) {
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
    info.is_custom = false;
    info.enabled = true;
    tool_info_[name] = info;

    itool_table_.emplace(name, std::move(tool));
}

// ---------------------------------------------------------------------------
// Unified execution: ITool first, then CommandFn fallback
// ---------------------------------------------------------------------------

Dictionary HandlerRegistry::execute(const String &name, const Dictionary &args) {
    // Check ITool table first
    auto it = itool_table_.find(name);
    if (it != itool_table_.end()) {
        return it->second->execute(args);
    }

    // Fall back to CommandFn table
    auto fn_it = table_.find(name);
    if (fn_it != table_.end()) {
        return fn_it->value(args);
    }

    // Tool not found
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
    return d;
}

Array HandlerRegistry::get_all_tools() const {
    Array result;
    for (const KeyValue<String, ToolInfo> &kv : tool_info_) {
        result.push_back(make_tool_entry(kv.value));
    }
    return result;
}

Array HandlerRegistry::get_enabled_tools() const {
    Array result;
    for (const KeyValue<String, ToolInfo> &kv : tool_info_) {
        if (kv.value.enabled) {
            result.push_back(make_tool_entry(kv.value));
        }
    }
    return result;
}

bool HandlerRegistry::is_tool_enabled(const String &name) const {
    auto it = tool_info_.find(name);
    return it != tool_info_.end() && it->value.enabled;
}

void HandlerRegistry::set_tool_enabled(const String &name, bool enabled) {
    auto it = tool_info_.find(name);
    if (it != tool_info_.end()) {
        it->value.enabled = enabled;
    }
}

// ---------------------------------------------------------------------------
// Category queries (for progressive disclosure)
// ---------------------------------------------------------------------------
//
// get_categories() 输出字段契约(每个分类节点):
//   id          : 分类段名(如 "node"、"property"),用于 get_tools 查询
//   name        : 分类展示名(如 "Node"、"Property"),客户端 UI 显示用
//   path        : 完整分类路径(如 "node/property"),用于 get_tools 查询
//   description : 分类描述(权威源,来自 top_level_meta 硬编码或工具 category_description())
//   tool_count  : 直接挂载到该分类的工具数(不含子分类)
//   total       : 包含子分类的累加值
//   subcategories : 子分类列表(数组),递归结构一致,若无则省略
namespace {

String prettify_segment(const String &seg) {
    if (seg.is_empty()) return seg;
    return seg.substr(0, 1).to_upper() + seg.substr(1);
}

struct TopLevelMeta {
    String label;
    String description;
    TopLevelMeta() = default;
    TopLevelMeta(const String &p_label, const String &p_desc) :
        label(p_label), description(p_desc) {}
};
const HashMap<String, TopLevelMeta> &top_level_meta() {
    static const HashMap<String, TopLevelMeta> kTopLevel = {
        {String("meta_tools"), TopLevelMeta(String::utf8("Meta Tools"),
                                            String::utf8("元工具与系统信息查询"))},
        {String("node_tools"), TopLevelMeta(String::utf8("Node Tools"),
                                            String::utf8("节点属性读取与修改工具，按 Godot 节点类型分类组织"))},
    };
    return kTopLevel;
}

}  // namespace

// 把 CatNode 序列化为 Dictionary(MCP 协议输出契约)。
//   id          : 分类段名(如 "node"、"property"),用于 get_tools 查询
//   name        : 分类展示名(如 "Node"、"Property"),客户端 UI 显示用
//   path        : 完整分类路径(如 "node/property"),用于 get_tools 查询
//   description : 分类描述(权威源,来自 top_level_meta 硬编码或工具 category_description())
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
        String label;       // 顶级可被硬编码 meta 覆盖;非顶级为空,序列化时 prettify
        int direct = 0;       // 直接挂载数(不含子分类)
        int total = 0;        // 累加数(含子分类)
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

    // 顶级节点应用硬编码 meta(label + description)
    // 这是权威源,覆盖任何 catDesc/brief 反填
    const HashMap<String, TopLevelMeta> &top_meta = top_level_meta();
    for (KeyValue<String, CatNode> &kv : root.children) {
        const TopLevelMeta *m = top_meta.getptr(kv.key);
        if (m) {
            if (!m->label.is_empty()) kv.value.label = m->label;
            if (!m->description.is_empty()) kv.value.description = m->description;
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
                const String name = child.label.is_empty()
                                        ? prettify_segment(child.name)
                                        : child.label;
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

const ToolInfo *HandlerRegistry::get_tool_schema(const String &name) const {
    return find_tool_info(name);
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

}  // namespace godot_mcp
