#include "tool_base.hpp"
#include "cmd_utils.hpp"
#include "logging.hpp"

using namespace godot;

namespace godot_mcp {

// =========================================================================
// ToolResult
// =========================================================================

Dictionary ToolResult::ok(const Dictionary &data) {
    Dictionary r;
    r["success"] = true;
    if (data.size() > 0) {
        r["data"] = data;
    }
    return r;
}

Dictionary ToolResult::ok_with_meta(const Dictionary &data, const Dictionary &meta) {
    Dictionary r;
    r["success"] = true;
    if (data.size() > 0) {
        r["data"] = data;
    }
    if (meta.size() > 0) {
        r["meta"] = meta;
    }
    return r;
}

Dictionary ToolResult::ok_with_confirm(const Dictionary &data, const String &confirm_message) {
    Dictionary r;
    r["success"] = true;
    if (data.size() > 0) {
        r["data"] = data;
    }
    r["confirm"] = confirm_message;
    return r;
}

Dictionary ToolResult::err(const String &code, const String &message) {
    Dictionary error;
    error["code"] = code;
    error["message"] = message;
    Dictionary r;
    r["success"] = false;
    r["error"] = error;
    return r;
}

Dictionary ToolResult::err_with_recoverable(const String &code, const String &message, const String &suggestion) {
    Dictionary error;
    error["code"] = code;
    error["message"] = message;
    Dictionary r;
    r["success"] = false;
    r["error"] = error;
    r["recoverable"] = true;
    r["suggestion"] = suggestion;
    return r;
}

// =========================================================================
// ITool::execute — 模板方法
// =========================================================================

Dictionary ITool::execute(const Dictionary &args) {
    ToolContext ctx;
    ctx.args = args;

    // ── Input Schema Validation ──
    {
        Dictionary schema = input_schema();
        if (schema.has("required")) {
            Array required = schema["required"];
            for (int i = 0; i < required.size(); i++) {
                String param_name = required[i];
                if (!ctx.args.has(param_name)) {
                    return ToolResult::err("MISSING_REQUIRED_PARAM",
                        String("Missing required parameter: ") + param_name);
                }
            }
        }
        if (schema.has("properties")) {
            Dictionary props = schema["properties"];
            Array param_names = props.keys();
            for (int i = 0; i < param_names.size(); i++) {
                String pname = param_names[i];
                if (!ctx.args.has(pname)) continue;
                Variant val = ctx.args[pname];
                Dictionary prop_def = props[pname];
                if (prop_def.has("type")) {
                    String expected_type = prop_def["type"];
                    bool type_ok = true;
                    if (expected_type == "string" && val.get_type() != Variant::STRING && val.get_type() != Variant::NIL)
                        type_ok = false;
                    else if (expected_type == "integer" && val.get_type() != Variant::INT && val.get_type() != Variant::FLOAT)
                        type_ok = false;
                    else if (expected_type == "number" && val.get_type() != Variant::FLOAT && val.get_type() != Variant::INT)
                        type_ok = false;
                    else if (expected_type == "boolean" && val.get_type() != Variant::BOOL)
                        type_ok = false;
                    else if (expected_type == "array" && val.get_type() != Variant::ARRAY)
                        type_ok = false;
                    else if (expected_type == "object" && val.get_type() != Variant::DICTIONARY)
                        type_ok = false;

                    if (!type_ok) {
                        return ToolResult::err("INVALID_PARAM_TYPE",
                            String("Parameter '") + pname + "' expected type " + expected_type +
                            " but got " + Variant::get_type_name(val.get_type()));
                    }
                }
            }
        }
    }

    // ── 场景根节点解析 ──
    if (needs_scene()) {
        Dictionary old_err;
        ctx.root = get_root_or_error(old_err);
        if (!ctx.root) {
            return ToolResult::err("NO_SCENE",
                old_err.get("error", "No scene is currently open in the editor"));
        }
    }

    // ── 节点路径解析 ──
    if (needs_node()) {
        String node_path = args.get("node_path", "");
        if (node_path.is_empty()) {
            node_path = args.get("path", "");
        }
        if (node_path.is_empty()) {
            return ToolResult::err("MISSING_PARAM",
                "Tool needs 'node_path' or 'path' parameter");
        }
        if (!ctx.root) {
            ctx.root = get_root();
        }
        if (!ctx.root) {
            return ToolResult::err("NO_SCENE", "No scene is currently open");
        }
        ctx.node = resolve_node(ctx.root, node_path);
        if (!ctx.node) {
            return ToolResult::err("NODE_NOT_FOUND",
                "Node not found: " + node_path);
        }
    }

    // ── 执行业务逻辑 ──
    Dictionary result = execute_impl(ctx);

    // ── 安全包裹：确保统一返回信封 ──
    if (!result.has("success")) {
        log_warn("tool", name() + String(": execute_impl returned dict without 'success' key, auto-wrapping"));
        Dictionary wrapped;
        wrapped["success"] = true;
        wrapped["data"] = result;
        return wrapped;
    }

    return result;
}

} // namespace godot_mcp
