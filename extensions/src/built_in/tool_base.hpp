#pragma once

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>

namespace godot_mcp {

using godot::Array;
using godot::Callable;
using godot::ClassDB;
using godot::Dictionary;
using godot::Error;
using godot::Node;
using godot::Object;
using godot::PackedStringArray;
using godot::String;
using godot::Variant;

class HandlerRegistry; // 前向声明，避免循环依赖

// ── ToolResult: 统一返回信封 ──
//   success: {"success": true, "data": {...}}
//   failure: {"success": false, "error": {"code": "...", "message": "..."}}
class ToolResult {
public:
    static godot::Dictionary ok(const godot::Dictionary &data = {});
    static godot::Dictionary ok_with_meta(const godot::Dictionary &data, const godot::Dictionary &meta);
    static godot::Dictionary ok_with_confirm(const godot::Dictionary &data, const godot::String &confirm_message);
    static godot::Dictionary err(const godot::String &code, const godot::String &message);
    static godot::Dictionary err_with_recoverable(const godot::String &code, const godot::String &message, const godot::String &suggestion);
};

// ── ToolContext: 前置检查后注入的上下文 ──
struct ToolContext {
    godot::Node *root = nullptr;
    godot::Node *node = nullptr;
    godot::Dictionary args;
};

// ── ITool: 所有工具的统一接口 ──
class ITool {
public:
    virtual ~ITool() = default;

    // ── 元数据 ──
    virtual godot::String name() const = 0;
    virtual godot::String brief() const = 0;
    virtual godot::String description() const = 0;
    virtual godot::Dictionary input_schema() const = 0;

    // ── 两轴分类 ──
    // category() 返回分组 key（如 "scene", "node"），用于 list_tool_categories 归类
    virtual godot::String category() const = 0;
    virtual godot::String category_description() const { return {}; }

    // is_meta() 控制 tools/list 可见性
    //    true  → 始终可见（发现工具：list_tools/list_tool_categories 等）
    //    false → 渐进式披露（通过 list_tool_categories 发现后再调用 list_tools 展开）
    virtual bool is_meta() const { return false; }

    // ── 能力声明（组合优于继承）──
    virtual bool needs_scene() const { return false; }
    virtual bool needs_node() const { return false; }
    virtual bool supports_undo() const { return false; }
    virtual bool is_destructive() const { return is_destructive_; }
    void set_is_destructive(bool v) { is_destructive_ = v; }

    // ── 依赖注入 ──
    // HandlerRegistry 在注册时调用此方法注入自身指针，meta 工具需要用它回调查询
    virtual void set_registry(HandlerRegistry * /*reg*/) {}

    // ── 统一入口（模板方法）──
    // 自动执行前置检查（root/node 解析）、调用 execute_impl、包裹统一信封
    godot::Dictionary execute(const godot::Dictionary &args);

protected:
    // 子类实现业务逻辑，ctx 中 root/node 已保证非空（如果声明了 needs_scene/needs_node）
    virtual godot::Dictionary execute_impl(const ToolContext &ctx) = 0;
    bool is_destructive_ = false;
};

} // namespace godot_mcp
