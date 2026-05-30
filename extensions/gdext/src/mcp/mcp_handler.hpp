#pragma once

#include "../commands/handler_registry.hpp"

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

    // io_session_id is in/out: for initialize, the newly created session ID is
    // written back into this reference so the transport layer can return it
    // in the MCP-Session-Id response header.
    Dictionary handle_message(const Dictionary &jsonrpc_msg, String &io_session_id);

    String create_session();
    bool destroy_session(const String &session_id);
    bool validate_session(const String &session_id) const;
    bool is_session_initialized(const String &session_id) const;
    Array get_active_sessions() const;

    bool has_pending_events(const String &session_id) const;
    Dictionary consume_event(const String &session_id);

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
    struct Session {
        String id;
        String protocol_version;
        Dictionary capabilities;
        Dictionary client_info;
        bool initialized = false;
        double created_at;
        int log_level = 3; // RFC 5424: Warning=3
        Vector<Dictionary> sse_event_queue;
    };

    Session *find_session(const String &id);

    static String generate_uuid();
    static Dictionary make_jsonrpc_response(const Variant &id, const Variant &result);
    static Dictionary make_jsonrpc_result(const Variant &id, const Dictionary &result);
    static Dictionary make_jsonrpc_error(const Variant &id, int code, const String &message,
                                         const Variant &data = {});
    static Dictionary make_notification(const String &method, const Variant &params);

    static Array tool_result_to_mcp_content(const Dictionary &tool_result);

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

    // Prompts
    Dictionary handle_prompts_list(const Variant &id);
    Dictionary handle_prompts_get(const Dictionary &params, const Variant &id);

    // Utilities
    Dictionary handle_logging_setLevel(const String &session_id, const Dictionary &params, const Variant &id);
    Dictionary handle_completion_complete(const Dictionary &params, const Variant &id);

    // Notifications (no return value needed)
    void handle_cancelled(const Dictionary &params);
    void handle_progress(const Dictionary &params);

    HandlerRegistry *registry_;
    HashMap<String, Session> sessions_;
    HashMap<String, String> pending_requests_; // request id (as string) -> session id, for cancellation
    HashMap<String, Variant> cancelled_requests_; // session_id -> request ids that are cancelled
};

} // namespace godot_mcp
