#include "tool_base.hpp"
#include "cmd_utils.hpp"

using namespace godot;

namespace godot_mcp {

// =========================================================================
// ToolResult
// =========================================================================

Dictionary ToolResult::ok(Dictionary data) {
    Dictionary r;
    r["success"] = true;
    if (data.size() > 0) {
        r["data"] = data;
    }
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

bool ToolResult::is_ok(const Dictionary &r) {
    return r.has("success") && r["success"].operator bool();
}

bool ToolResult::is_err(const Dictionary &r) {
    return r.has("success") && !r["success"].operator bool();
}

// =========================================================================
// ITool::execute — 模板方法
// =========================================================================

Dictionary ITool::execute(const Dictionary &args) {
    ToolContext ctx;
    ctx.args = args;

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
        Dictionary wrapped;
        wrapped["success"] = true;
        wrapped["data"] = result;
        return wrapped;
    }

    return result;
}

} // namespace godot_mcp
