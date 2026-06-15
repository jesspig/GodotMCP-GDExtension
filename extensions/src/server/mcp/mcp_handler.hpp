#pragma once

#include "tool_executor.hpp"
#include "../registry/handler_registry.hpp"

#include <functional>
#include <map>
#include <memory>

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>
#include "prompt_provider.hpp"

namespace godot_mcp {
using namespace godot;

class McpHandler {
public:
    McpHandler(HandlerRegistry *registry);

    // io_session_id is in/out: for initialize, the newly created session ID is
    // written back into this reference so the transport layer can return it
    // in the MCP-Session-Id response header.
    Dictionary handle_message(const Dictionary &jsonrpc_msg, String &io_session_id);

    String create_session();
    bool destroy_session(const String &session_id);
    bool validate_session(const String &session_id) const;

    bool has_pending_events(const String &session_id) const;
    Dictionary consume_event(const String &session_id);

    // Notify all initialized sessions that the tool list has changed
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
    bool has_pending_requests() const;
    int pending_request_count() const;
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

    static constexpr int kMaxSseQueue = 1000;
    static constexpr double kSessionTtl = 3600.0;
    static constexpr int kMaxSessions = 16;

private:
    struct Session {
        String id;
        String protocol_version;
        Dictionary client_info;
        bool initialized = false;
        double last_activity = 0.0;
        Vector<Dictionary> sse_event_queue;
    };

    Session *find_session(const String &id);
    void cleanup_expired_sessions();

    static String generate_uuid();
    static Dictionary make_jsonrpc_response(const Variant &id, const Variant &result);
    static Dictionary make_jsonrpc_result(const Variant &id, const Dictionary &result);
    static Dictionary make_jsonrpc_error(const Variant &id, int code, const String &message,
                                         const Variant &data = {});
    static Dictionary make_notification(const String &method, const Variant &params);

    void enqueue_event(const String &session_id, const Dictionary &event);

    // Lifecycle
    // session_id is in/out: when a new session is created, the ID is written back here.
    Dictionary handle_initialize(String &session_id, const Dictionary &params, const Variant &id);
    Dictionary handle_ping(const Variant &id);

    // Tools
    Dictionary handle_tools_list(const String &session_id, const Dictionary &params, const Variant &id);
    Dictionary handle_tools_call(const String &session_id, const Dictionary &params, const Variant &id);

    // Resources
    Dictionary handle_resources_list(const Variant &id);
    Dictionary handle_resources_read(const Dictionary &params, const Variant &id);
    Dictionary handle_resources_templates_list(const Variant &id);
    Dictionary _build_scene_tree_node(Node *node) const;

    // Utilities
    Dictionary handle_completion_complete(const Dictionary &params, const Variant &id);

    // Notifications (no return value needed)
    void handle_cancelled(const Dictionary &params, const String &session_id);

    HandlerRegistry *registry_;
    std::map<String, std::unique_ptr<Session>> sessions_;
    // Keyed by a composite "session\x1F<serialized_id>" so that null-id
    // requests and same-numbered ids from different sessions no longer collide
    // (the old key was just JSON::stringify(id), which collapsed all null-ids
    // and cross-session same ids into one slot).
    HashMap<String, String> pending_requests_;
    HashMap<String, Variant> cancelled_requests_; // session_id -> request ids that are cancelled
    McpLogCallback log_callback_;
    ToolExecutor tool_executor_;
    PromptProvider prompt_provider_;
};

} // namespace godot_mcp
