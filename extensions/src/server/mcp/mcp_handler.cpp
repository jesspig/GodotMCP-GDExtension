#include "mcp_handler.hpp"

#include "built_in/cmd_utils.hpp"
#include "logging.hpp"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <charconv>
#include <cstdlib>
#include <cstring>
#include <random>

using namespace godot;

namespace godot_mcp {

// -------------------------------------------------------------------------
// Construction
// -------------------------------------------------------------------------
McpHandler::McpHandler(HandlerRegistry *registry)
    : registry_(registry) {}

// -------------------------------------------------------------------------
// Session management
// -------------------------------------------------------------------------
String McpHandler::generate_uuid() {
    static std::mt19937_64 rng(std::random_device{}());
    static std::uniform_int_distribution<uint64_t> dist;

    char buf[37];
    const uint64_t hi = dist(rng);
    const uint64_t lo = dist(rng);
    // RFC 4122 version 4
    std::sprintf(buf, "%08x-%04x-4%03x-%04x-%012llx",
                 (unsigned)(hi >> 32),
                 (unsigned)(hi >> 16) & 0xffff,
                 (unsigned)(hi & 0x0fff),
                 (unsigned)(lo >> 48) & 0x3fff | 0x8000,
                 (unsigned long long)(lo & 0x0000ffffffffffffULL));
    return String(buf);
}

String McpHandler::create_session() {
    Session s;
    s.id = generate_uuid();
    s.created_at = Time::get_singleton()->get_unix_time_from_system();
    s.log_level = 3; // Warning
    sessions_[s.id] = s;
    return s.id;
}

bool McpHandler::destroy_session(const String &session_id) {
    auto it = sessions_.find(session_id);
    if (it == sessions_.end()) return false;
    sessions_.erase(it->key);
    return true;
}

bool McpHandler::validate_session(const String &session_id) const {
    return sessions_.has(session_id);
}

bool McpHandler::is_session_initialized(const String &session_id) const {
    auto it = sessions_.find(session_id);
    return it != sessions_.end() && it->value.initialized;
}

Array McpHandler::get_active_sessions() const {
    Array ids;
    for (const KeyValue<String, Session> &kv : sessions_) {
        ids.push_back(kv.key);
    }
    return ids;
}

McpHandler::Session *McpHandler::find_session(const String &id) {
    auto it = sessions_.find(id);
    return it != sessions_.end() ? &it->value : nullptr;
}

// -------------------------------------------------------------------------
// SSE event queue
// -------------------------------------------------------------------------
bool McpHandler::has_pending_events(const String &session_id) const {
    auto it = sessions_.find(session_id);
    return it != sessions_.end() && !it->value.sse_event_queue.is_empty();
}

Dictionary McpHandler::consume_event(const String &session_id) {
    Session *s = find_session(session_id);
    if (!s || s->sse_event_queue.is_empty()) return Dictionary();
    Dictionary ev = s->sse_event_queue[0];
    s->sse_event_queue.remove_at(0);
    return ev;
}

void McpHandler::enqueue_event(const String &session_id, const Dictionary &event) {
    Session *s = find_session(session_id);
    if (s) {
        s->sse_event_queue.push_back(event);
    }
}

// -------------------------------------------------------------------------
// JSON-RPC 2.0 helpers
// -------------------------------------------------------------------------
Dictionary McpHandler::make_jsonrpc_response(const Variant &id, const Variant &result) {
    Dictionary d;
    d["jsonrpc"] = "2.0";
    d["id"] = id;
    d["result"] = result;
    return d;
}

Dictionary McpHandler::make_jsonrpc_result(const Variant &id, const Dictionary &result) {
    return make_jsonrpc_response(id, Variant(result));
}

Dictionary McpHandler::make_jsonrpc_error(const Variant &id, int code,
                                           const String &message, const Variant &data) {
    Dictionary d;
    d["jsonrpc"] = "2.0";
    d["id"] = id;
    Dictionary err;
    err["code"] = code;
    err["message"] = message;
    if (data.get_type() != Variant::NIL) {
        err["data"] = data;
    }
    d["error"] = err;
    return d;
}

Dictionary McpHandler::make_notification(const String &method, const Variant &params) {
    Dictionary d;
    d["jsonrpc"] = "2.0";
    d["method"] = method;
    if (params.get_type() != Variant::NIL) {
        d["params"] = params;
    }
    return d;
}

// -------------------------------------------------------------------------
// Tool result → MCP content array
// -------------------------------------------------------------------------
Array McpHandler::tool_result_to_mcp_content(const Dictionary &tool_result) {
    Array content;
    Dictionary text_item;
    text_item["type"] = "text";
    text_item["text"] = JSON::stringify(tool_result);
    content.push_back(text_item);
    return content;
}

// -------------------------------------------------------------------------
// Protocol version negotiation
// -------------------------------------------------------------------------
String McpHandler::negotiate_protocol_version(const String &header_value) {
    static const Vector<String> kSupported = {
        "2025-11-25",
        "2025-03-26"
    };
    if (header_value.is_empty()) {
        return "2025-03-26";
    }
    for (int i = 0; i < kSupported.size(); ++i) {
        if (header_value == kSupported[i]) {
            return header_value;
        }
    }
    // Fallback: assume client can handle 2025-03-26
    return "2025-03-26";
}

// -------------------------------------------------------------------------
// Main message dispatch — the heart of the MCP protocol
// -------------------------------------------------------------------------
// Returns JSON-RPC response Dictionary (empty for notifications).
// io_session_id is an in/out parameter: if the message creates a session
// (initialize), the new session ID is written back here.
Dictionary McpHandler::handle_message(const Dictionary &jsonrpc_msg, String &io_session_id) {
    const Variant id_v = jsonrpc_msg.has("id") ? jsonrpc_msg["id"] : Variant();
    const bool is_request = id_v.get_type() != Variant::NIL;

    const String version = jsonrpc_msg.get("jsonrpc", "");
    if (!version.is_empty() && version != "2.0") {
        return make_jsonrpc_error(id_v, kInvalidRequest,
                                   String("Unsupported JSON-RPC version: ") + version);
    }

    const String method = jsonrpc_msg.get("method", "");
    if (method.is_empty()) {
        return make_jsonrpc_error(id_v, kInvalidRequest, "Missing 'method' field");
    }

    const Dictionary params = jsonrpc_msg.has("params") && jsonrpc_msg["params"].get_type() == Variant::DICTIONARY
                                  ? Dictionary(jsonrpc_msg["params"])
                                  : Dictionary();

    Session *session = nullptr;
    if (!io_session_id.is_empty()) {
        session = find_session(io_session_id);
    }

    if (method == "initialize") {
        return handle_initialize(io_session_id, params, id_v);
    }

    // All other methods need a valid session
    if (!session) {
        return make_jsonrpc_error(id_v, kInvalidRequest, "No valid session. Call initialize first.");
    }

    // Lifecycle
    if (method == "ping") {
        return handle_ping(id_v);
    }
    if (method == "notifications/initialized") {
        session->initialized = true;
        return Dictionary();
    }

    // Tools
    if (method == "tools/list") {
        return handle_tools_list(io_session_id, params, id_v);
    }
    if (method == "tools/call") {
        return handle_tools_call(io_session_id, params, id_v);
    }

    // Resources
    if (method == "resources/list") {
        return handle_resources_list(id_v);
    }
    if (method == "resources/read") {
        return handle_resources_read(params, id_v);
    }
    if (method == "resources/templates/list") {
        return handle_resources_templates_list(id_v);
    }

    // Prompts
    if (method == "prompts/list") {
        return handle_prompts_list(id_v);
    }
    if (method == "prompts/get") {
        return handle_prompts_get(params, id_v);
    }

    // Utilities
    if (method == "logging/setLevel") {
        return handle_logging_setLevel(io_session_id, params, id_v);
    }
    if (method == "completion/complete") {
        return handle_completion_complete(params, id_v);
    }

    // Notifications
    if (method == "notifications/cancelled") {
        handle_cancelled(params);
        return Dictionary();
    }
    if (method == "notifications/progress") {
        handle_progress(params);
        return Dictionary();
    }

    // Unknown method
    return make_jsonrpc_error(id_v, kMethodNotFound, String("Unknown method: ") + method);
}

// -------------------------------------------------------------------------
// initialize
// -------------------------------------------------------------------------
Dictionary McpHandler::handle_initialize(String &session_id, const Dictionary &params,
                                          const Variant &id) {
    Session *session = find_session(session_id);
    if (!session) {
        session_id = create_session();
        session = find_session(session_id);
    }

    const String client_version = params.get("protocolVersion", "");
    session->protocol_version = negotiate_protocol_version(client_version);
    session->client_info = params.get("clientInfo", Dictionary());
    session->capabilities = params.get("capabilities", Dictionary());

    log_info("mcp", String("Initialize from '") +
                     JSON::stringify(session->client_info) +
                     String("', protocol ") + session->protocol_version);

    Dictionary caps;
    Dictionary tools_caps;
    tools_caps["listChanged"] = true;
    caps["tools"] = tools_caps;
    caps["resources"] = Dictionary();
    caps["prompts"] = Dictionary();
    caps["logging"] = Dictionary();
    caps["completions"] = Dictionary();

    Dictionary server_info;
    server_info["name"] = "godot-mcp";
    server_info["version"] = registry_ ? registry_->plugin_version() : "0.0.0";

    Dictionary result;
    result["protocolVersion"] = session->protocol_version;
    result["capabilities"] = caps;
    result["serverInfo"] = server_info;

    return make_jsonrpc_result(id, result);
}

// -------------------------------------------------------------------------
// ping
// -------------------------------------------------------------------------
Dictionary McpHandler::handle_ping(const Variant &id) {
    return make_jsonrpc_response(id, Dictionary());
}

// -------------------------------------------------------------------------
// tools/list — returns only always-on tools (progressive disclosure)
// -------------------------------------------------------------------------
Dictionary McpHandler::handle_tools_list(const String &session_id, const Dictionary &params,
                                          const Variant &id) {
    if (!registry_) {
        return make_jsonrpc_error(id, kInternalError, "Registry not initialized");
    }
    Array always_on = registry_->get_always_on_tools();

    Dictionary result;
    result["tools"] = always_on;
    return make_jsonrpc_result(id, result);
}

// -------------------------------------------------------------------------
// tools/call
// -------------------------------------------------------------------------
Dictionary McpHandler::handle_tools_call(const String &session_id, const Dictionary &params,
                                          const Variant &id) {
    if (!registry_) {
        return make_jsonrpc_error(id, kInternalError, "Registry not initialized");
    }

    const String tool_name = params.get("name", "");
    if (tool_name.is_empty()) {
        return make_jsonrpc_error(id, kInvalidParams, "Missing 'name' in tool call");
    }

    const Dictionary args = params.has("arguments") && params["arguments"].get_type() == Variant::DICTIONARY
                                ? Dictionary(params["arguments"])
                                : Dictionary();

    log_info("mcp", String("tools/call: ") + tool_name);

    // Log key parameters (limit to first 200 chars to avoid flooding)
    {
        String param_log;
        Array param_keys = args.keys();
        for (int i = 0; i < param_keys.size(); i++) {
            if (!param_log.is_empty()) param_log += ", ";
            String k = param_keys[i];
            Variant v = args[k];
            param_log += k + String("=");
            if (v.get_type() == Variant::STRING) {
                String sv = v;
                if (sv.length() > 80) sv = sv.substr(0, 80) + String("...");
                param_log += String("\"") + sv + String("\"");
            } else {
                param_log += json_stringify_safe(v);
            }
        }
        if (param_log.length() > 200) param_log = param_log.substr(0, 200) + String("...");
        log_info("mcp", String("  args: ") + param_log);
    }

    const Variant meta = params.get("_meta", Variant());
    String progress_token;
    if (meta.get_type() == Variant::DICTIONARY) {
        const Variant pt = Dictionary(meta).get("progressToken", Variant());
        if (pt.get_type() != Variant::NIL) {
            progress_token = pt;
        }
    }

    // Track as pending for cancellation support
    pending_requests_[JSON::stringify(id)] = session_id;

    // Check cancellation
    {
        auto cancel_it = cancelled_requests_.find(session_id);
        if (cancel_it != cancelled_requests_.end()) {
            const Variant canceled_id = cancel_it->value;
            if (canceled_id.get_type() == Variant::INT && (int64_t)canceled_id == (int64_t)id) {
                cancelled_requests_.erase(session_id);
                return make_jsonrpc_error(id, kServerTerminated, "Request cancelled");
            }
        }
    }

    // Execute — 统一调度：先查 ITool 表，再查 CommandFn 表
    Dictionary tool_result;
    try {
        tool_result = registry_->execute(tool_name, args);
    } catch (const std::exception &e) {
        pending_requests_.erase(JSON::stringify(id));
        log_warn("mcp", String("tools/call FAILED (exception): ") + tool_name + String(" - ") + String(e.what()));
        return make_jsonrpc_error(id, kInternalError, String(e.what()));
    }

    pending_requests_.erase(JSON::stringify(id));

    // Log result summary
    {
        bool success = true;
        String summary;
        if (tool_result.has("error")) {
            success = false;
            Variant err_val = tool_result["error"];
            if (err_val.get_type() == Variant::DICTIONARY) {
                Dictionary ed = err_val;
                summary = String("error=") + String(ed.get("code", "?")) + String(": ") + String(ed.get("message", "?"));
            } else {
                summary = String("error=") + String(err_val);
            }
        } else if (tool_result.has("success") && !tool_result["success"].operator bool()) {
            success = false;
            summary = String("success=false");
        } else if (tool_result.has("data") && tool_result["data"].get_type() == Variant::DICTIONARY) {
            Dictionary data = tool_result["data"];
            Array dk = data.keys();
            for (int i = 0; i < dk.size() && summary.length() < 120; i++) {
                if (!summary.is_empty()) summary += ", ";
                String k = dk[i];
                Variant v = data[k];
                summary += k + String("=");
                if (v.get_type() == Variant::STRING) {
                    String sv = v;
                    if (sv.length() > 60) sv = sv.substr(0, 60) + String("...");
                    summary += String("\"") + sv + String("\"");
                } else {
                    summary += json_stringify_safe(v);
                }
            }
        }
        if (success) {
            log_info("mcp", String("  -> ok: ") + summary);
        } else {
            log_warn("mcp", String("  -> FAIL: ") + summary);
        }
    }

    // 错误检测：兼容旧格式 {"error": "..."} 和新格式 {"success": false, "error": {"code": "...", "message": "..."}}
    if (tool_result.has("error")) {
        const Variant err_val = tool_result["error"];
        String err_msg;
        if (err_val.get_type() == Variant::DICTIONARY) {
            const Dictionary err_dict = err_val;
            err_msg = err_dict.get("message", "Unknown error");
        } else {
            err_msg = err_val;
        }
        return make_jsonrpc_error(id, kInternalError, err_msg);
    }
    if (tool_result.has("success") && !tool_result["success"].operator bool()) {
        return make_jsonrpc_error(id, kInternalError, "Tool execution failed");
    }

    Dictionary result;
    result["content"] = tool_result_to_mcp_content(tool_result);
    result["isError"] = false;

    // 透传其他字段（兼容旧行为）
    const Array dict_keys = tool_result.keys();
    for (int i = 0; i < dict_keys.size(); ++i) {
        const String k = dict_keys[i];
        if (k != "error" && k != "success" && k != "data") {
            result[k] = tool_result[k];
        }
    }

    // 展开 data 中的字段到顶层
    if (tool_result.has("data") && tool_result["data"].get_type() == Variant::DICTIONARY) {
        const Dictionary data = tool_result["data"];
        const Array data_keys = data.keys();
        for (int i = 0; i < data_keys.size(); ++i) {
            const String k = data_keys[i];
            if (!result.has(k)) {
                result[k] = data[k];
            }
        }
    }

    return make_jsonrpc_result(id, result);
}

// -------------------------------------------------------------------------
// resources/list
// -------------------------------------------------------------------------
Dictionary McpHandler::handle_resources_list(const Variant &id) {
    Dictionary result;
    result["resources"] = Array();
    return make_jsonrpc_result(id, result);
}

// -------------------------------------------------------------------------
// resources/read
// -------------------------------------------------------------------------
Dictionary McpHandler::handle_resources_read(const Dictionary &params, const Variant &id) {
    const String uri = params.get("uri", "");
    return make_jsonrpc_error(id, kResourceNotFound,
                               String("Resource not found: ") + uri);
}

// -------------------------------------------------------------------------
// resources/templates/list
// -------------------------------------------------------------------------
Dictionary McpHandler::handle_resources_templates_list(const Variant &id) {
    Dictionary result;
    result["resourceTemplates"] = Array();
    return make_jsonrpc_result(id, result);
}

// -------------------------------------------------------------------------
// prompts/list
// -------------------------------------------------------------------------
Dictionary McpHandler::handle_prompts_list(const Variant &id) {
    Dictionary result;
    result["prompts"] = Array();
    return make_jsonrpc_result(id, result);
}

// -------------------------------------------------------------------------
// prompts/get
// -------------------------------------------------------------------------
Dictionary McpHandler::handle_prompts_get(const Dictionary &params, const Variant &id) {
    return make_jsonrpc_error(id, kInvalidParams, "No prompts available");
}

// -------------------------------------------------------------------------
// logging/setLevel
// -------------------------------------------------------------------------
Dictionary McpHandler::handle_logging_setLevel(const String &session_id, const Dictionary &params,
                                                 const Variant &id) {
    const String level_str = params.get("level", "");
    // RFC 5424: debug=7, info=6, notice=5, warning=4, error=3, critical=2, alert=1, emergency=0
    static const HashMap<String, int> level_map = {
        {"emergency", 0}, {"alert", 1}, {"critical", 2}, {"error", 3},
        {"warning", 4}, {"notice", 5}, {"info", 6}, {"debug", 7}
    };

    Session *s = find_session(session_id);
    if (!s) {
        return make_jsonrpc_error(id, kInvalidRequest, "Session not found");
    }

    auto it = level_map.find(level_str);
    if (it == level_map.end()) {
        s->log_level = 3; // default to error
    } else {
        s->log_level = it->value;
    }

    return make_jsonrpc_response(id, Dictionary());
}

// -------------------------------------------------------------------------
// completion/complete
// -------------------------------------------------------------------------
Dictionary McpHandler::handle_completion_complete(const Dictionary &params, const Variant &id) {
    Dictionary completion;
    completion["values"] = Array();
    completion["total"] = 0;
    completion["hasMore"] = false;

    Dictionary result;
    result["completion"] = completion;
    return make_jsonrpc_result(id, result);
}

// -------------------------------------------------------------------------
// notifications/cancelled
// -------------------------------------------------------------------------
void McpHandler::handle_cancelled(const Dictionary &params) {
    const Variant req_id = params.get("requestId", Variant());
    if (req_id.get_type() == Variant::NIL) return;

    // Find which session this request belongs to
    auto it = pending_requests_.find(JSON::stringify(req_id));
    if (it != pending_requests_.end()) {
        const String sid = it->value;
        cancelled_requests_[sid] = req_id;
        pending_requests_.erase(it->key);
    }
}

// -------------------------------------------------------------------------
// notifications/progress — MCP progress tracking (reserved for future use)
// -------------------------------------------------------------------------
void McpHandler::handle_progress(const Dictionary &params) {
    (void)params; // notification received, progress tracking TBD
}

// -------------------------------------------------------------------------
// notify_tools_list_changed — send notification to all initialized sessions
// -------------------------------------------------------------------------
void McpHandler::notify_tools_list_changed() {
    const Dictionary notification = make_notification(
        "notifications/tools/list_changed", Dictionary());
    for (KeyValue<String, Session> &kv : sessions_) {
        if (kv.value.initialized) {
            enqueue_event(kv.key, notification);
        }
    }
}

} // namespace godot_mcp
