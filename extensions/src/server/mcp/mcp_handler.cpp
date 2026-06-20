#include "mcp_handler.hpp"

#include "logging.hpp"
#include "runtime/bridge_server.hpp"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <array>

using namespace godot;

namespace godot_mcp {

// -------------------------------------------------------------------------
// Construction
// -------------------------------------------------------------------------
McpHandler::McpHandler(HandlerRegistry *registry)
    : registry_(registry), tool_executor_(*registry) {
    // tool_executor_ ctor dereferences registry — must be non-null at construction
    CRASH_COND(registry == nullptr);
}

void McpHandler::set_log_callback(McpLogCallback cb) {
    log_callback_ = std::move(cb);
}

// -------------------------------------------------------------------------
// SSE event queue
// -------------------------------------------------------------------------
bool McpHandler::has_pending_events() const {
    return !global_event_queue_.empty();
}

Dictionary McpHandler::consume_event() {
    if (global_event_queue_.empty()) return Dictionary();
    Dictionary ev = global_event_queue_[0];
    global_event_queue_.pop_front();
    return ev;
}

void McpHandler::enqueue_event(const Dictionary &event) {
    global_event_queue_.push_back(event);
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
// Protocol version negotiation
// -------------------------------------------------------------------------
String McpHandler::negotiate_protocol_version(const String &header_value) {
    static constexpr std::array<const char*, 3> kSupported = {
        "2026-07-28",
        "2025-11-25",
        "2025-03-26"
    };
    if (header_value.is_empty()) {
        return "2025-03-26";
    }
    for (const auto &ver : kSupported) {
        if (header_value == String(ver)) {
            return header_value;
        }
    }
    // Fallback: assume client can handle 2025-03-26
    return "2025-03-26";
}

// -------------------------------------------------------------------------
// Main message dispatch — the heart of the MCP protocol
// -------------------------------------------------------------------------
Dictionary McpHandler::handle_message(const Dictionary &jsonrpc_msg) {
    const Variant id_v = jsonrpc_msg.has("id") ? jsonrpc_msg["id"] : Variant();

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

    if (method == "server/discover") {
        return handle_server_discover(id_v);
    }

    if (method == "initialize") {
        return handle_initialize(params, id_v);
    }

    // Lifecycle
    if (method == "ping") {
        return handle_ping(id_v);
    }

    // Tools
    if (method == "tools/list") {
        return handle_tools_list(params, id_v);
    }
    if (method == "tools/call") {
        return handle_tools_call(params, id_v);
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
        Dictionary result;
        result["prompts"] = prompt_provider::list_prompts();
        return make_jsonrpc_result(id_v, result);
    }
    if (method == "prompts/get") {
        const String name = params.get("name", "");
        Dictionary args_dict = params.has("arguments") && params["arguments"].get_type() == Variant::DICTIONARY
                                   ? Dictionary(params["arguments"])
                                   : Dictionary();
        Dictionary prompt_result = prompt_provider::get_prompt(name, args_dict);
        if (prompt_result.is_empty()) {
            return make_jsonrpc_error(id_v, kInvalidParams, "No such prompt: " + name);
        }
        return make_jsonrpc_result(id_v, prompt_result);
    }

    // Utilities
    if (method == "completion/complete") {
        return handle_completion_complete(params, id_v);
    }

    // Logging
    if (method == "logging/setLevel") {
        return handle_logging_set_level(params, id_v);
    }

    // Notifications
    if (method == "notifications/cancelled") {
        handle_cancelled(params);
        return Dictionary();
    }
    if (method == "notifications/initialized") {
        return Dictionary();
    }

    // Unknown method
    return make_jsonrpc_error(id_v, kMethodNotFound, String("Unknown method: ") + method);
}

// -------------------------------------------------------------------------
// initialize (MCP standard lifecycle)
// -------------------------------------------------------------------------
Dictionary McpHandler::handle_initialize(const Dictionary &params, const Variant &id) {
    const String client_version = params.get("protocolVersion", "");
    const String negotiated = negotiate_protocol_version(client_version);

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
    result["protocolVersion"] = negotiated;
    result["serverInfo"] = server_info;
    result["capabilities"] = caps;

    return make_jsonrpc_result(id, result);
}

// -------------------------------------------------------------------------
// server/discover
// -------------------------------------------------------------------------
Dictionary McpHandler::handle_server_discover(const Variant &id) {
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

    Array supported_versions;
    supported_versions.push_back(String("2026-07-28"));
    supported_versions.push_back(String("2025-11-25"));
    supported_versions.push_back(String("2025-03-26"));

    Dictionary result;
    result["supportedVersions"] = supported_versions;
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
// logging/setLevel
// -------------------------------------------------------------------------
Dictionary McpHandler::handle_logging_set_level(const Dictionary &params, const Variant &id) {
    const String level = params.get("level", "");

    static const char *valid_levels[] = {
        "debug", "info", "warning", "error", "critical", "off"
    };
    bool valid = false;
    for (const char *l : valid_levels) {
        if (level == String(l)) {
            valid = true;
            break;
        }
    }
    if (!valid) {
        return make_jsonrpc_error(id, kInvalidParams,
            String("Invalid log level: ") + level);
    }

    log_level_ = level;
    return make_jsonrpc_result(id, Dictionary());
}

// -------------------------------------------------------------------------
// tools/list — returns only always-on tools (progressive disclosure)
// -------------------------------------------------------------------------
Dictionary McpHandler::handle_tools_list(const Dictionary & /*params*/, const Variant &id) {
    if (!registry_) {
        return make_jsonrpc_error(id, kInternalError, "Registry not initialized");
    }

    Dictionary result;
    result["tools"] = registry_->get_always_on_tools();
    return make_jsonrpc_result(id, result);
}

// -------------------------------------------------------------------------
// tools/call
// -------------------------------------------------------------------------
Dictionary McpHandler::handle_tools_call(const Dictionary &params, const Variant &id) {
    const String tool_name = params.get("name", "");
    if (tool_name.is_empty()) {
        return make_jsonrpc_error(id, kInvalidParams, "Missing 'name' in tool call");
    }

    const Dictionary args = params.has("arguments") && params["arguments"].get_type() == Variant::DICTIONARY
                                ? Dictionary(params["arguments"])
                                : Dictionary();

    // Unwrap call_tool to check the actual tool's destructiveness
    String effective_tool_name = tool_name;
    Dictionary effective_args = args;
    if (tool_name == "call_tool" && args.has("tool_name")) {
        effective_tool_name = args["tool_name"];
        if (args.has("arguments") && args["arguments"].get_type() == Variant::DICTIONARY) {
            effective_args = args["arguments"];
        }
    }

    // Destructive tool interception (checks effective tool, not call_tool wrapper)
    if (registry_) {
        const ToolInfo *info = registry_->find_tool_info(effective_tool_name);
        if (info && info->is_destructive && !allow_all_for_session_) {
            PendingDestructiveOp op;
            op.jsonrpc_id = id;
            op.tool_name = effective_tool_name;
            op.arguments = effective_args;
            op.created_msec = Time::get_singleton()->get_ticks_msec();
            pending_ops_.push_back(std::move(op));

            if (confirm_callback_) {
                confirm_callback_(pending_ops_.back());
            }

            return Dictionary();
        }
    }

    // Delegate to ToolExecutor for execution, logging, and MCP formatting
    Dictionary exec_result = tool_executor_.execute(tool_name, args, id);

    // Structured log callback
    if (log_callback_) {
        ToolCallLog log_entry;
        log_entry.timestamp = Time::get_singleton()->get_datetime_string_from_system();
        log_entry.tool_name = effective_tool_name;
        log_entry.success = !exec_result.has("_exec_error");
        log_entry.args = args;
        log_entry.result = ToolExecutor::extract_result(exec_result);
        log_entry.duration_ms = exec_result.get("_exec_duration_ms", 0.0);
        log_callback_(log_entry);
    }

    // Error path: ToolExecutor signalled a protocol-level error
    if (exec_result.has("_exec_error")) {
        const Dictionary err_info = exec_result["_exec_error"];
        return make_jsonrpc_error(id,
            err_info.get("code", kInternalError),
            err_info.get("message", "Unknown error"),
            err_info.get("data", Variant()));
    }

    // Pending bridge request — poll bridge server immediately for a fast synchronous response.
    // For fast commands (scene tree, properties, input) the response arrives within 1 frame.
    if (exec_result.has("pending") &&
        exec_result["pending"].get_type() == Variant::INT) {
        const int64_t pending_id = static_cast<int64_t>(exec_result["pending"]);
        if (bridge_server_) {
            // Poll the bridge server to process any incoming responses
            bridge_server_->poll();
            // Check if the response for this pending request has been queued
            // The response_cb_ will have already enqueued an event if data arrived.
        }
        // A brief poll is done; if data arrived, it's in the event queue.
        // Otherwise, return pending info — client can wait for SSE.
        Dictionary result_content;
        result_content["pending"] = pending_id;
        result_content["message"] = "Bridge request in progress, response will arrive via SSE event";
        Array content;
        Dictionary text_item;
        text_item["type"] = "text";
        text_item["text"] = JSON::stringify(result_content);
        content.push_back(text_item);
        Dictionary mcp_result;
        mcp_result["content"] = content;
        mcp_result["isError"] = false;
        return make_jsonrpc_result(id, mcp_result);
    }

    // Strip internal keys and use as JSON-RPC result
    exec_result.erase("_raw_result");
    exec_result.erase("_exec_duration_ms");
    return make_jsonrpc_result(id, exec_result);
}

// -------------------------------------------------------------------------
// resources/list
// -------------------------------------------------------------------------
Dictionary McpHandler::handle_resources_list(const Variant &id) {
    Array resources;

    Dictionary scene_tree_res;
    scene_tree_res["uri"] = "godot://scene-tree";
    scene_tree_res["name"] = "Scene Tree";
    scene_tree_res["description"] = "Current scene tree structure as JSON";
    scene_tree_res["mimeType"] = "application/json";
    resources.append(scene_tree_res);

    Dictionary proj_settings_res;
    proj_settings_res["uri"] = "godot://project-settings";
    proj_settings_res["name"] = "Project Settings";
    proj_settings_res["description"] = "Project configuration settings";
    proj_settings_res["mimeType"] = "application/json";
    resources.append(proj_settings_res);

    Dictionary editor_info_res;
    editor_info_res["uri"] = "godot://editor-info";
    editor_info_res["name"] = "Editor Info";
    editor_info_res["description"] = "Editor version, state, and capabilities";
    editor_info_res["mimeType"] = "application/json";
    resources.append(editor_info_res);

    Dictionary result;
    result["resources"] = resources;
    return make_jsonrpc_result(id, result);
}

// -------------------------------------------------------------------------
// resources/read
// -------------------------------------------------------------------------
Dictionary McpHandler::handle_resources_read(const Dictionary &params, const Variant &id) {
    const String uri = params.get("uri", "");

    // Handle template-based URIs first (must check before exact match)
    if (uri.begins_with("godot://scene-node/")) {
        String path = uri.substr(18); // length of "godot://scene-node/"
        EditorInterface *ei = EditorInterface::get_singleton();
        if (ei) {
            Node *root = ei->get_edited_scene_root();
            if (root) {
                Node *target = root->get_node<Node>(NodePath(path));
                if (target) {
                    Dictionary data = _build_scene_tree_node(target);
                    Array contents;
                    Dictionary text_item;
                    text_item["type"] = "text";
                    text_item["text"] = JSON::stringify(data);
                    contents.append(text_item);
                    Dictionary result;
                    result["contents"] = contents;
                    return make_jsonrpc_result(id, result);
                }
            }
        }
        Dictionary empty_data;
        empty_data["message"] = "Node not found";
        Array contents;
        Dictionary text_item;
        text_item["type"] = "text";
        text_item["text"] = JSON::stringify(empty_data);
        contents.append(text_item);
        Dictionary result;
        result["contents"] = contents;
        return make_jsonrpc_result(id, result);
    }

    if (uri == "godot://scene-tree") {
        Dictionary data;
        EditorInterface *ei = EditorInterface::get_singleton();
        if (ei) {
            Node *root = ei->get_edited_scene_root();
            if (root) {
                data = _build_scene_tree_node(root);
            } else {
                data["message"] = "No scene open";
            }
        } else {
            data["message"] = "Editor interface not available";
        }
        Array contents;
        Dictionary text_item;
        text_item["type"] = "text";
        text_item["text"] = JSON::stringify(data);
        contents.append(text_item);
        Dictionary result;
        result["contents"] = contents;
        return make_jsonrpc_result(id, result);
    }

    if (uri == "godot://project-settings") {
        Dictionary data;
        ProjectSettings *ps = ProjectSettings::get_singleton();
        if (ps) {
            data["project_name"] = ps->get_setting("application/config/name");
            data["description"] = ps->get_setting("application/config/description");
            data["version"] = ps->get_setting("application/config/version");
            data["boot_splash"] = ps->get_setting("application/boot_splash/image");
            data["icon"] = ps->get_setting("application/config/icon");
            data["window_size"] = ps->get_setting("display/window/size/viewport_width");
            data["window_height"] = ps->get_setting("display/window/size/viewport_height");
            data["window_mode"] = ps->get_setting("display/window/size/mode");
            data["vsync"] = ps->get_setting("display/window/vsync/vsync_mode");
            data["renderer"] = ps->get_setting("rendering/renderer/rendering_method");
        }
        Array contents;
        Dictionary text_item;
        text_item["type"] = "text";
        text_item["text"] = JSON::stringify(data);
        contents.append(text_item);
        Dictionary result;
        result["contents"] = contents;
        return make_jsonrpc_result(id, result);
    }

    if (uri == "godot://editor-info") {
        Dictionary data;
        EditorInterface *ei = EditorInterface::get_singleton();
        if (ei) {
            data["has_scene_open"] = ei->get_edited_scene_root() != nullptr;
            data["is_playing"] = ei->is_playing_scene();
            data["editor_version"] = "Godot 4.6+";
        }
        Engine *engine = Engine::get_singleton();
        if (engine) {
            Dictionary vi = engine->get_version_info();
            data["engine_version"] = vi;
        }
        data["plugin_version"] = registry_ ? registry_->plugin_version() : "unknown";
        Array contents;
        Dictionary text_item;
        text_item["type"] = "text";
        text_item["text"] = JSON::stringify(data);
        contents.append(text_item);
        Dictionary result;
        result["contents"] = contents;
        return make_jsonrpc_result(id, result);
    }

    return make_jsonrpc_error(id, kResourceNotFound, String("Resource not found: ") + uri);
}

// -------------------------------------------------------------------------
// resources/templates/list
// -------------------------------------------------------------------------
Dictionary McpHandler::handle_resources_templates_list(const Variant &id) {
    Array templates;

    Dictionary scene_node_tpl;
    scene_node_tpl["uriTemplate"] = "godot://scene-node/{path}";
    scene_node_tpl["name"] = "Scene Node";
    scene_node_tpl["description"] = "Get info about a specific node in the scene tree by its path";
    scene_node_tpl["mimeType"] = "application/json";
    templates.append(scene_node_tpl);

    Dictionary result;
    result["resourceTemplates"] = templates;
    return make_jsonrpc_result(id, result);
}

// -------------------------------------------------------------------------
// completion/complete
// -------------------------------------------------------------------------
Dictionary McpHandler::handle_completion_complete(const Dictionary &params, const Variant &id) {
    Dictionary completion;
    completion["values"] = Array();
    completion["total"] = 0;
    completion["hasMore"] = false;

    if (registry_) {
        const Dictionary argument = params.get("argument", Dictionary());
        const String current_value = argument.get("value", "");
        if (!current_value.is_empty()) {
            const Array suggestions = registry_->get_search_suggestions(current_value, 10);
            completion["values"] = suggestions;
            completion["total"] = suggestions.size();
            completion["hasMore"] = suggestions.size() >= 10;
        }
    }

    Dictionary result;
    result["completion"] = completion;
    return make_jsonrpc_result(id, result);
}

// -------------------------------------------------------------------------
// notifications/cancelled
// -------------------------------------------------------------------------
void McpHandler::handle_cancelled(const Dictionary & /*params*/) {
    // Synchronous execution model: cancellations are handled by the
    // server polling loop, no action needed at this layer.
}

// -------------------------------------------------------------------------
// notify_tools_list_changed — push notification to global event queue
// -------------------------------------------------------------------------
void McpHandler::notify_tools_list_changed() {
    const Dictionary notification = make_notification(
        "notifications/tools/list_changed", Dictionary());
    enqueue_event(notification);
}

// -------------------------------------------------------------------------
// _build_scene_tree_node — recursive helper for scene-tree resource
// -------------------------------------------------------------------------
Dictionary McpHandler::_build_scene_tree_node(Node *node, int depth, int max_depth, int max_children) const {
    Dictionary d;
    d["name"] = node->get_name();
    d["type"] = node->get_class();
    d["node_path"] = node->get_path();
    d["child_count"] = node->get_child_count();

    if (depth >= max_depth) {
        d["truncated"] = true;
        return d;
    }

    Array children;
    const int child_count = node->get_child_count();
    const int limit = child_count < max_children ? child_count : max_children;
    for (int i = 0; i < limit; i++) {
        Node *child = Object::cast_to<Node>(node->get_child(i));
        if (child) {
            children.append(_build_scene_tree_node(child, depth + 1, max_depth, max_children));
        }
    }
    if (child_count > max_children) {
        d["truncated"] = true;
    }
    d["children"] = children;
    return d;
}

// -------------------------------------------------------------------------
// resolve_pending_op — process user confirmation for destructive operation
// -------------------------------------------------------------------------
void McpHandler::resolve_pending_op(const Variant &id, bool allow, bool allow_all_for_session) {
    if (allow_all_for_session) {
        allow_all_for_session_ = true;
    }

    for (auto it = pending_ops_.begin(); it != pending_ops_.end(); ++it) {
        if (it->jsonrpc_id != id) continue;
        if (it->state != PendingDestructiveOp::State::Pending) continue;

        it->state = PendingDestructiveOp::State::Resolved;

        if (allow) {
            Dictionary exec_result = tool_executor_.execute(it->tool_name, it->arguments);

            if (log_callback_) {
                ToolCallLog log_entry;
                log_entry.timestamp = Time::get_singleton()->get_datetime_string_from_system();
                log_entry.tool_name = it->tool_name;
                log_entry.success = !exec_result.has("_exec_error");
                log_entry.args = it->arguments;
                log_entry.result = ToolExecutor::extract_result(exec_result);
                log_entry.duration_ms = exec_result.get("_exec_duration_ms", 0.0);
                log_callback_(log_entry);
            }

            Dictionary jsonrpc_resp;
            if (exec_result.has("_exec_error")) {
                const Dictionary err_info = exec_result["_exec_error"];
                jsonrpc_resp = make_jsonrpc_error(it->jsonrpc_id,
                    err_info.get("code", kInternalError),
                    err_info.get("message", "Unknown error"),
                    err_info.get("data", Variant()));
            } else {
                exec_result.erase("_raw_result");
                exec_result.erase("_exec_duration_ms");
                jsonrpc_resp = make_jsonrpc_result(it->jsonrpc_id, exec_result);
            }
            enqueue_event(jsonrpc_resp);
        } else {
            if (log_callback_) {
                ToolCallLog log_entry;
                log_entry.timestamp = Time::get_singleton()->get_datetime_string_from_system();
                log_entry.tool_name = it->tool_name;
                log_entry.success = false;
                log_entry.args = it->arguments;
                log_entry.duration_ms = 0.0;
                log_callback_(log_entry);
            }
            Dictionary err_resp = make_jsonrpc_error(it->jsonrpc_id, kInvalidRequest,
                String("Destructive tool denied by user: ") + it->tool_name);
            enqueue_event(err_resp);
        }

        pending_ops_.erase(it);
        return;
    }
}

void McpHandler::check_pending_timeouts(uint64_t timeout_msec) {
    const uint64_t now = Time::get_singleton()->get_ticks_msec();
    std::deque<PendingDestructiveOp> still_pending;

    for (auto &op : pending_ops_) {
        if (op.state != PendingDestructiveOp::State::Pending) continue;
        if (now - op.created_msec > timeout_msec) {
            Dictionary err_resp = make_jsonrpc_error(op.jsonrpc_id, kInvalidRequest,
                String("Destructive tool timed out (no user response): ") + op.tool_name);
            enqueue_event(err_resp);
        } else {
            still_pending.push_back(std::move(op));
        }
    }
    pending_ops_ = std::move(still_pending);
}

} // namespace godot_mcp
