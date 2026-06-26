#pragma once

#include <optional>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <string>
#include <godot_cpp/variant/variant.hpp>

namespace godot_mcp {

// Own plugin path, auto-detected during McpEditorPlugin::_enter_tree().
// Used by enable/disable_plugin tools to prevent self-disable.
// Falls back to "res://addons/godot_mcp" if call("get_plugin_path") fails.
// NOTE: std::string, not godot::String �� must be safe for static init
// (before GDExtension API is available).
extern std::string g_mcp_self_plugin_path;

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

class HandlerRegistry; // ǰ������������ѭ����??

// ���� ToolResult: ͳһ�����ŷ� ����
//   success: {"success": true, "data": {...}}
//   failure: {"success": false, "error": {"code": "...", "message": "..."}}
class ToolResult {
public:
    [[nodiscard]] static godot::Dictionary ok(const godot::Dictionary &data = {});
    [[nodiscard]] static godot::Dictionary err(const godot::String &code, const godot::String &message);
};

// ���� ToolContext: ǰ�ü���ע��������� ����
struct ToolContext {
    godot::Node *root = nullptr;
    godot::Node *node = nullptr;
    godot::Dictionary args;
    godot::Variant jsonrpc_id;
};

// ���� ITool: ���й��ߵ�ͳһ�ӿ� ����
class ITool {
public:
    virtual ~ITool() = default;

    // ���� Ԫ��??����
    virtual godot::String name() const = 0;
    virtual godot::String brief() const = 0;
    virtual godot::String description() const { return brief(); }
    godot::Dictionary input_schema() const;
    virtual godot::Dictionary build_input_schema() const {
        godot::Dictionary s;
        s["type"] = "object";
        s["properties"] = godot::Dictionary();
        return s;
    }

    // ���� ������� ����
    // category() ���ط��� key���� "scene", "node"�������� list_tool_categories ����
    virtual godot::String category() const = 0;
    virtual godot::String category_description() const { return {}; }

    // is_meta() ���� tools/list �ɼ�??
    //    true  ??ʼ�տɼ������ֹ��ߣ�list_tools/list_tool_categories �ȣ�
    //    false ??����ʽ��¶��ͨ�� list_tool_categories ���ֺ��ٵ��� list_tools չ��??
    virtual bool is_meta() const { return false; }

    // tool_group() returns a domain label like "animation", "ui", "filesystem"
    // Used by get_tools_by_group() for grouped queries.
    // Auto-derived from the last segment of category() — tools can override for custom grouping.
    virtual godot::String tool_group() const {
        String cat = category();
        int last_slash = cat.rfind("/");
        if (last_slash >= 0) {
            return cat.substr(last_slash + 1);
        }
        return cat;
    }

    // ���� ����������������ڼ̳У�����
    virtual bool needs_scene() const { return false; }
    virtual bool needs_node() const { return false; }
    virtual bool supports_undo() const { return false; }
    virtual bool is_destructive() const { return is_destructive_; }
    void set_is_destructive(bool v) { is_destructive_ = v; }

    // ���� ����ע�� ����
    // HandlerRegistry ��ע��ʱ���ô˷���ע������ָ�룬meta ������Ҫ�����ص���??
    virtual void set_registry(HandlerRegistry * /*reg*/) {}

    // ���� ͳһ��ڣ�ģ�巽��������
    // �Զ�ִ��ǰ�ü�飨root/node ����������??execute_impl������ͳһ�ŷ�
    godot::Dictionary execute(const godot::Dictionary &args, const godot::Variant &jsonrpc_id = {});

protected:
    // ����ʵ��ҵ���߼���ctx ??root/node �ѱ�֤�ǿգ��������??needs_scene/needs_node??
    virtual godot::Dictionary execute_impl(const ToolContext &ctx) = 0;
    bool is_destructive_ = false;

private:
    mutable std::optional<godot::Dictionary> schema_cache_;
};

} // namespace godot_mcp
