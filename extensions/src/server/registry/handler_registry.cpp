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
//   name        : 分类 ID(段名,如 "node"、"property"),用于 list_tools_in_category 查询
//   label       : 分类展示名(如 "Node"、"Property"),客户端 UI 显示用
//   description : 分类描述(权威源,来自 top_level_meta 硬编码或工具 category_description())
//   tool_count  : 直接挂载到该分类的工具数(不含子分类)。
//                 客户端 UI 可用于显示小角标或完全忽略。
//   total       : 包含子分类的累加值(向后兼容)
//   subcategories : 子分类列表(数组),若无则省略
//
// 17 个原始分类被 category_remap() 重组到 6 个顶级:
//   node     (54 工具): collision / find / node / property/2d / property/3d
//   scene    (16 工具): scene
//   editor   (10 工具): editor_control / search
//   script   (14 工具): script/csharp / script_gd / script_helpers
//   settings (21 工具): settings/core / settings/extended / input_map
//   other    ( 9 工具): meta / plugin_management / undo

namespace {

// 把分类段名转成展示名
//   "2d" -> "2D", "3d" -> "3D"
//   "csharp" -> "C#", "gdscript" -> "GDScript"
//   "input_map" -> "Input Map", "plugin_management" -> "Plugin Management"
//   "operation" -> "Operation", "project" -> "Project", "general" -> "General"
//   其他 -> 首字母大写
String prettify_segment(const String &seg) {
    if (seg == "2d") return String("2D");
    if (seg == "3d") return String("3D");
    if (seg == "csharp") return String("C#");
    if (seg == "gdscript") return String("GDScript");
    if (seg == "input_map") return String("Input Map");
    if (seg == "plugin_management") return String("Plugin Management");
    if (seg == "operation") return String("Operation");
    if (seg == "project") return String("Project");
    if (seg == "general") return String("General");
    if (seg.is_empty()) return seg;
    // 首字母大写,其他字符保留(godot-cpp 无 char_uppercase 静态方法)
    return seg.substr(0, 1).to_upper() + seg.substr(1);
}

// 把工具的原始 category 路径映射到新顶级分类下的路径
// 例: "property/2d" -> "node/property/2d"
//     "settings/core" -> "settings/project"
//     "input_map" -> "settings/input_map"
//     "meta" -> "other/meta"
const HashMap<String, String> &category_remap() {
    static const HashMap<String, String> kRemap = {
        // node 顶级
        {String("collision"),         String("node/collision")},
        {String("find"),              String("node/find")},
        {String("node"),              String("node/operation")},
        {String("property/2d"),       String("node/property/2d")},
        {String("property/3d"),       String("node/property/3d")},
        // scene 顶级
        {String("scene"),             String("scene")},
        // editor 顶级
        {String("editor_control"),    String("editor/general")},
        {String("search"),            String("editor/search")},
        // script 顶级
        {String("script/csharp"),     String("script/csharp")},
        {String("script_gd"),         String("script/gdscript")},
        {String("script_helpers"),    String("script/helpers")},
        // settings 顶级
        {String("settings/core"),     String("settings/project")},
        {String("settings/extended"), String("settings/extended")},
        {String("input_map"),         String("settings/input_map")},
        // other 顶级
        {String("meta"),              String("other/meta")},
        {String("plugin_management"), String("other/plugin_management")},
        {String("undo"),              String("other/undo")},
    };
    return kRemap;
}

// 顶级分类的硬编码元数据(label + description)
// 6 个顶级,desc 作为权威源,避免依赖"子分类反填"或"工具 brief 兜底"。
//
// 字符串必须用 String::utf8() 包裹:handler_registry.cpp 是 UTF-8 无 BOM,
// MSVC 默认按系统 GBK 解释源文件中的非 ASCII 字节,不显式标记会导致运行时
// 中文乱码。
struct TopLevelMeta {
    String label;
    String description;
    TopLevelMeta() = default;
    TopLevelMeta(const String &p_label, const String &p_desc) :
        label(p_label), description(p_desc) {}
};
const HashMap<String, TopLevelMeta> &top_level_meta() {
    static const HashMap<String, TopLevelMeta> kTopLevel = {
        {String("node"),     TopLevelMeta(String::utf8("Node"),
                                          String::utf8("节点的创建、操作与管理"))},
        {String("scene"),    TopLevelMeta(String::utf8("Scene"),
                                          String::utf8("场景的创建、保存与操作"))},
        {String("editor"),   TopLevelMeta(String::utf8("Editor"),
                                          String::utf8("编辑器控制与信息查询"))},
        {String("script"),   TopLevelMeta(String::utf8("Script"),
                                          String::utf8("脚本的创建、编辑与管理"))},
        {String("settings"), TopLevelMeta(String::utf8("Settings"),
                                          String::utf8("项目设置(显示、输入、物理层等)"))},
        {String("other"),    TopLevelMeta(String::utf8("Other"),
                                          String::utf8("元工具、插件管理、撤销等辅助工具"))},
    };
    return kTopLevel;
}

}  // namespace

// 把 CatNode 序列化为 Dictionary(MCP 协议输出契约)。
//   name        : 分类 ID(段名,如 "node"、"property"),用于 list_tools_in_category 查询
//   label       : 分类展示名(如 "Node"、"Property"),客户端 UI 显示用
//   description : 分类描述(权威源,来自 top_level_meta 硬编码或工具 category_description())
//   tool_count  : 直接挂载到该分类的工具数(不含子分类)。
//                 这虽然指向"工具"而非"分类",但仍是分类节点的属性——
//                 表示"该分类下直接挂载的工具数",不包含子分类内的工具。
//                 客户端 UI 可用于显示"(tool_count)"小角标或完全忽略。
//   total       : 包含子分类的累加值(向后兼容;若客户端想算总数可读此字段)
//   subcategories : 子分类列表(数组),若无则省略
static Dictionary cat_node_to_dict(const String &name, const String &label,
                                   int direct, int total,
                                   const String &description,
                                   const Array &subs) {
    Dictionary d;
    d["name"] = name;
    d["label"] = label.is_empty() ? name : label;
    d["tool_count"] = direct;
    d["total"] = total;
    if (!description.is_empty())
        d["description"] = description;
    if (!subs.is_empty())
        d["subcategories"] = subs;
    return d;
}

// 分类路径映射：显式映射优先，node_prop/* 动态映射次之，原样传递兜底
static String remap_category(const String &orig_cat) {
    const String *remapped = category_remap().getptr(orig_cat);
    if (remapped) {
        return *remapped;
    }
    // Catch-all: node_prop/<path> → node/property/<path>
    static const String kNodePropPrefix("node_prop/");
    if (orig_cat.begins_with(kNodePropPrefix)) {
        return String("node/property/") + orig_cat.substr(kNodePropPrefix.length());
    }
    return orig_cat;
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

        // 应用 category remap:把工具的原始 category 路径映射到新顶级分类
        // 例: "property/2d" -> "node/property/2d"
        //     "node_prop/Node/CanvasItem" -> "node/property/Node/CanvasItem"
        const String cat = remap_category(orig_cat);

        const PackedStringArray segments = cat.split("/");
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
    std::function<Array(const CatNode &)> node_to_subs =
        [&](const CatNode &parent) -> Array {
            std::vector<Dictionary> entries;
            for (const KeyValue<String, CatNode> &kv : parent.children) {
                const CatNode &child = kv.value;
                const Array subs = child.children.is_empty()
                                       ? Array()
                                       : node_to_subs(child);
                // label: 硬编码优先,否则 prettify
                const String label = child.label.is_empty()
                                         ? prettify_segment(child.name)
                                         : child.label;
                entries.push_back(cat_node_to_dict(
                    child.name, label, child.direct, child.total,
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

    return node_to_subs(root);
}

Array HandlerRegistry::get_tools_in_category(const String &category) const {
    Array result;
    const String prefix = String(category) + String("/");
    for (const KeyValue<String, ToolInfo> &kv : tool_info_) {
        if (!kv.value.enabled) continue;
        // 应用 category remap,使客户端能按新顶级路径查询
        // 例: 调用 "node/operation" 会查到原始 category = "node" 的 21 个工具
        //     "node_prop/Node/CanvasItem" -> "node/property/Node/CanvasItem"
        const String tool_cat = remap_category(kv.value.category);
        if (tool_cat == category || tool_cat.begins_with(prefix)) {
            Dictionary d;
            d["name"] = kv.value.name;
            d["brief"] = kv.value.brief;
            result.push_back(d);
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
