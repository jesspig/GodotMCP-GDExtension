#pragma once

#include "tool_executor.hpp"
#include "prompt_provider.hpp"
#include "pending_operation.hpp"
#include "../registry/handler_registry.hpp"

#include <deque>
#include <functional>

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>

namespace godot_mcp {
using namespace godot;

class McpHandler {
public:
    McpHandler(HandlerRegistry *registry);

    Dictionary handle_message(const Dictionary &jsonrpc_msg);

    bool has_pending_events() const;
    Dictionary consume_event();

    // Notify all consumers that the tool list has changed
    void notify_tools_list_changed();

    // Structured log data for tool calls
    struct ToolCallLog {
        String timestamp;    // ISO 8601 "2026-06-13T14:30:00"
        String tool_name;
        bool success;
        Dictionary args;     // 输入参数
        Dictionary result;   // 响应数据
        double duration_ms;
    };

    using McpLogCallback = std::function<void(const ToolCallLog &)>;
    void set_log_callback(McpLogCallback cb);

    using ConfirmCallback = std::function<void(const PendingDestructiveOp &)>;
    void set_confirm_callback(ConfirmCallback cb) { confirm_callback_ = std::move(cb); }
    void resolve_pending_op(const godot::Variant &id, bool allow, bool allow_all_for_session);
    void check_pending_timeouts(uint64_t timeout_msec);
    void reset_session_flags() { allow_all_for_session_ = false; }
    int pending_op_count() const noexcept { return static_cast<int>(pending_ops_.size()); }

    // Utility: parse a MCP-Protocol-Version header and return a compatible version.
    static String negotiate_protocol_version(const String &header_value);

    // JSON-RPC 2.0 standard error codes
    static constexpr int kParseError = -32700;
    static constexpr int kInvalidRequest = -32600;
    static constexpr int kMethodNotFound = -32601;
    static constexpr int kInvalidParams = -32602;
    static constexpr int kInternalError = -32603;
    static constexpr int kResourceNotFound = -32002;
    static constexpr int kServerTerminated = -32001;

private:
    static Dictionary make_jsonrpc_response(const Variant &id, const Variant &result);
    static Dictionary make_jsonrpc_result(const Variant &id, const Dictionary &result);
    static Dictionary make_jsonrpc_error(const Variant &id, int code, const String &message,
                                         const Variant &data = {});
    static Dictionary make_notification(const String &method, const Variant &params);

    void enqueue_event(const Dictionary &event);

    // Lifecycle
    Dictionary handle_initialize(const Dictionary &params, const Variant &id);
    Dictionary handle_server_discover(const Variant &id);
    Dictionary handle_ping(const Variant &id);

    // Tools
    Dictionary handle_tools_list(const Dictionary & /*params*/, const Variant &id);
    Dictionary handle_tools_call(const Dictionary &params, const Variant &id);

    // Resources
    Dictionary handle_resources_list(const Variant &id);
    Dictionary handle_resources_read(const Dictionary &params, const Variant &id);
    Dictionary handle_resources_templates_list(const Variant &id);
    Dictionary _build_scene_tree_node(Node *node, int depth = 0, int max_depth = 15, int max_children = 200) const;

    // Utilities
    Dictionary handle_completion_complete(const Dictionary &params, const Variant &id);

    // Logging
    Dictionary handle_logging_set_level(const Dictionary &params, const Variant &id);

    // Notifications (no return value needed)
    void handle_cancelled(const Dictionary & /*params*/);

    HandlerRegistry *registry_;
    McpLogCallback log_callback_;
    ToolExecutor tool_executor_;
    std::deque<Dictionary> global_event_queue_;
    String log_level_ = "info";
    std::deque<PendingDestructiveOp> pending_ops_;
    bool allow_all_for_session_ = false;
    ConfirmCallback confirm_callback_;
};

} // namespace godot_mcp
