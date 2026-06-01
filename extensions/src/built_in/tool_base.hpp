#pragma once

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>

namespace godot_mcp {

class HandlerRegistry; // 前向声明，避免循环依赖

// ── ToolResult: 统一返回信封 ──
//   success: {"success": true, "data": {...}}
//   failure: {"success": false, "error": {"code": "...", "message": "..."}}
class ToolResult {
public:
    static godot::Dictionary ok(godot::Dictionary data = {});
    static godot::Dictionary err(const godot::String &code, const godot::String &message);
    static bool is_ok(const godot::Dictionary &r);
    static bool is_err(const godot::Dictionary &r);
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
    // category_label() 是分组展示名，默认同 category()，SDK 工具可覆盖
    virtual godot::String category_label() const { return category(); }

    // source() 返回 "meta" | "built_in" | "custom"
    //    meta → tools/list 始终可见
    //    built_in → 渐进式披露
    //    custom → 自动加 custom_ 前缀
    virtual godot::String source() const { return "built_in"; }

    // ── 能力声明（组合优于继承）──
    virtual bool needs_scene() const { return false; }
    virtual bool needs_node() const { return false; }

    // ── 依赖注入 ──
    // HandlerRegistry 在注册时调用此方法注入自身指针，meta 工具需要用它回调查询
    virtual void set_registry(HandlerRegistry * /*reg*/) {}

    // ── 统一入口（模板方法）──
    // 自动执行前置检查（root/node 解析）、调用 execute_impl、包裹统一信封
    godot::Dictionary execute(const godot::Dictionary &args);

    // ── 注册名计算 ──
    godot::String registered_name() const {
        if (source() == "custom") return "custom_" + name();
        return name();
    }

protected:
    // 子类实现业务逻辑，ctx 中 root/node 已保证非空（如果声明了 needs_scene/needs_node）
    virtual godot::Dictionary execute_impl(const ToolContext &ctx) = 0;
};

} // namespace godot_mcp
