#include "http_server.hpp"
#include "test_http_handler.hpp"

#include "built_in/cmd_utils.hpp"
#include "logging.hpp"

#include <cstring>

#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

namespace godot_mcp {

HttpServer::HttpServer() = default;

HttpServer::~HttpServer() {
    stop();
}

Error HttpServer::start(uint16_t port, McpHandler *mcp_handler, const String &bind_address) {
    stop();
    mcp_handler_ = mcp_handler;
    port_ = port;
    bind_address_ = bind_address;

    tcp_server_.instantiate();
    const Error err = tcp_server_->listen(port, bind_address_);
    if (err != OK) {
        log_error("http", String("Failed to listen on ") + bind_address_ +
                              String(":") + String::num_int64(port) +
                              String(" (err=") + String::num_int64(static_cast<int64_t>(err)) + String(")"));
        tcp_server_.unref();
        return err;
    }

    auth_token_ = OS::get_singleton()->get_environment("GODOT_MCP_AUTH_TOKEN");
    if (!auth_token_.is_empty()) {
        log_info("http", "Token authentication enabled");
    }

    log_info("http",
             String("MCP Streamable HTTP on ") + bind_address_ + String(":") + String::num_int64(port));
    return OK;
}

void HttpServer::stop() {
    for (auto &kv : connections_) {
        if (kv.value.tcp.is_valid()) {
            kv.value.tcp->disconnect_from_host();
        }
    }
    connections_.clear();

    if (tcp_server_.is_valid()) {
        tcp_server_->stop();
        tcp_server_.unref();
    }
    mcp_handler_ = nullptr;
}

bool HttpServer::is_listening() const {
    return tcp_server_.is_valid() && tcp_server_->is_listening();
}

void HttpServer::finish_header_to_body(Connection &conn) {
    if (conn.content_length > 0 && conn.header_end_pos < conn.read_buf.size()) {
        const int remaining = static_cast<int>(conn.read_buf.size() - conn.header_end_pos);
        const int old_size = static_cast<int>(conn.body.size());
        conn.body.resize(old_size + remaining);
        memcpy(conn.body.ptrw() + old_size, conn.read_buf.ptr() + conn.header_end_pos, remaining);
    }
}

bool HttpServer::try_consume_rate() {
    const double now = Time::get_singleton()->get_ticks_msec() / 1000.0;
    const double elapsed = now - rate_last_refill_;
    if (elapsed > 0.0) {
        rate_tokens_ = std::min(kMaxTokens, rate_tokens_ + static_cast<int>(elapsed * kRefillRate));
        rate_last_refill_ = now;
    }
    if (rate_tokens_ > 0) {
        rate_tokens_--;
        return true;
    }
    return false;
}

// -------------------------------------------------------------------------
// Poll
// -------------------------------------------------------------------------
void HttpServer::poll() {
    if (!tcp_server_.is_valid() || !tcp_server_->is_listening()) return;
    if (polling_) return; // Re-entrancy guard (EditorProgress -> Main::iteration -> _process -> poll)
    polling_ = true;

    try {
    const uint64_t now = Time::get_singleton()->get_ticks_msec();

    check_timeouts();

    // Accept new connections
    while (tcp_server_->is_connection_available()) {
        if (connections_.size() >= kMaxConnections) {
            Ref<StreamPeerTCP> tcp = tcp_server_->take_connection();
            if (tcp.is_valid()) tcp->disconnect_from_host();
            log_warn("http", "Max connections reached (kicking)");
            continue;
        }
        Ref<StreamPeerTCP> tcp = tcp_server_->take_connection();
        if (tcp.is_null()) break;

        const int conn_id = next_conn_id_++;
        Connection conn;
        conn.tcp = tcp;
        conn.tcp->set_no_delay(true);
        conn.tcp->poll();
        conn.last_activity_msec = Time::get_singleton()->get_ticks_msec();
        connections_[conn_id] = conn;
    }

    // Poll existing connections
    Vector<int> dead;

    for (auto &kv : connections_) {
        const int conn_id = kv.key;
        Connection &conn = kv.value;
        if (conn.tcp.is_null()) { dead.push_back(conn_id); continue; }

        // SSE stream: flush events
        if (conn.is_sse_stream) {
            if (now - conn.sse_last_event_msec > kSseIdleTimeoutMsec) {
                dead.push_back(conn_id);
                continue;
            }
            if (conn.sse_write_errored) {
                dead.push_back(conn_id);
                continue;
            }
            flush_sse(conn_id, conn);
            const int64_t avail = conn.tcp->get_available_bytes();
            if (avail < 0) {
                dead.push_back(conn_id);
            }
            continue;
        }

        // --- Non-SSE: read available bytes ---
        conn.tcp->poll();
        const int64_t avail = conn.tcp->get_available_bytes();
        if (avail < 0) {
            dead.push_back(conn_id); continue;
        }

        // Read socket data into read_buf (only if data available)
        if (avail > 0) {
            conn.last_activity_msec = Time::get_singleton()->get_ticks_msec();

            if (!conn.headers_done) {
                const Array read_result = conn.tcp->get_data(static_cast<int>(avail));
                if (static_cast<Error>(static_cast<int>(read_result[0])) != OK) {
                    dead.push_back(conn_id); continue;
                }
                const PackedByteArray chunk = read_result[1];
                if (chunk.size() > 0) {
                    const int old = static_cast<int>(conn.read_buf.size());
                    conn.read_buf.resize(old + chunk.size());
                    std::copy_n(chunk.ptr(), chunk.size(), conn.read_buf.ptrw() + old);
                }

                ParseResult pr = parse_headers(conn);
                if (pr == ERROR_PARSE) {
                    if (conn.content_length > kMaxBodyLength) {
                        send_response(conn_id, conn, 413, "Payload Too Large", "text/plain",
                                      "413 Payload Too Large", "Connection: close\r\n");
                    } else {
                        send_response(conn_id, conn, 400, "Bad Request", "text/plain",
                                      "400 Bad Request", "Connection: close\r\n");
                    }
                    dead.push_back(conn_id);
                    continue;
                }
                if (pr == NEED_MORE) {
                    continue;
                }

                // Headers complete: copy any body bytes that arrived with headers
                finish_header_to_body(conn);
            } else {
                // Headers already done: read body data directly from socket
                if (conn.content_length > 0 && conn.body.size() < conn.content_length) {
                    const int remaining = static_cast<int>(conn.content_length - conn.body.size());
                    const int max_read = static_cast<int>(kMaxBodyLength - conn.body.size());
                    int to_read = static_cast<int>(avail) < remaining ? static_cast<int>(avail) : remaining;
                    if (to_read > max_read) to_read = max_read;
                    if (to_read <= 0) { dead.push_back(conn_id); continue; }
                    const Array read_result = conn.tcp->get_data(to_read);
                    if (static_cast<Error>(static_cast<int>(read_result[0])) != OK) {
                        dead.push_back(conn_id); continue;
                    }
                    const PackedByteArray read_chunk = read_result[1];
                    if (read_chunk.size() > 0) {
                        const int old = static_cast<int>(conn.body.size());
                        conn.body.resize(old + read_chunk.size());
                        std::copy_n(read_chunk.ptr(), read_chunk.size(), conn.body.ptrw() + old);
                    }
                }
            }
        }

        // Even if avail == 0, try parsing leftover read_buf when headers not done
        if (avail == 0 && !conn.headers_done && !conn.read_buf.is_empty()) {
            ParseResult pr = parse_headers(conn);
            if (pr == ERROR_PARSE) {
                send_response(conn_id, conn, 400, "Bad Request", "text/plain",
                              "400 Bad Request", "Connection: close\r\n");
                dead.push_back(conn_id);
                continue;
            }
            if (pr == COMPLETE) {
                finish_header_to_body(conn);
            } else {
                continue;
            }
        }

        // If no socket data and headers not done with no leftover, skip
        if (avail == 0 && !conn.headers_done && conn.read_buf.is_empty()) {
            continue;
        }

        // If no socket data and waiting for more body, skip
        if (avail == 0 && conn.headers_done && conn.body.size() < conn.content_length) {
            continue;
        }

        // Also try draining any remaining data from socket into body
        if (conn.content_length > 0 && conn.body.size() < conn.content_length) {
            try_read_body(conn);
        }

        // Body complete �?dispatch
        if (conn.headers_done && (conn.content_length <= 0 || conn.body.size() >= conn.content_length)) {
            // Non-SSE connections close after each request (poll architecture can't
            // reliably handle rapid-fire keepalive due to frame timing).
            if (!conn.is_sse_stream) {
                conn.keep_alive = false;
            }
            dispatch_request(conn_id, conn);
            if (!conn.is_sse_stream && connections_.has(conn_id)) {
                dead.push_back(conn_id);
            }
        }
    }

    for (int i = 0; i < dead.size(); ++i) close_connection(dead[i]);
    } catch (...) {
        polling_ = false;
        throw;
    }
    polling_ = false;
}

// -------------------------------------------------------------------------
// Request dispatch
// -------------------------------------------------------------------------
void HttpServer::dispatch_request(int conn_id, Connection &conn) {
    // Token authentication (constant-time comparison to prevent timing side-channel)
    if (!auth_token_.is_empty()) {
        String auth = conn.headers.has("authorization") ? conn.headers["authorization"] : "";
        if (auth.length() != auth_token_.length() + 7) {
            send_response(conn_id, conn, 401, "Unauthorized", "application/json",
                          "{\"error\":\"Missing or invalid Authorization header\"}");
            return;
        }
        CharString auth_utf8 = auth.utf8();
        CharString expected_utf8 = (String("Bearer ") + auth_token_).utf8();
        bool match = true;
        for (int i = 0; i < expected_utf8.length(); ++i) {
            if (auth_utf8[i] != expected_utf8[i]) match = false;
        }
        if (!match) {
            send_response(conn_id, conn, 401, "Unauthorized", "application/json",
                          "{\"error\":\"Missing or invalid Authorization header\"}");
            return;
        }
    }

    // Global rate limiting
    if (!try_consume_rate()) {
        send_response(conn_id, conn, 429, "Too Many Requests", "application/json",
                      "{\"error\":\"Rate limit exceeded (30 req/s)\"}");
        return;
    }

    if (conn.method == "POST") handle_post(conn_id, conn);
    else if (conn.method == "GET") handle_get(conn_id, conn);
    else if (conn.method == "DELETE") handle_delete(conn_id, conn);
    else if (conn.method == "OPTIONS") handle_options(conn_id, conn);
    else {
        log_warn("http", String("Unsupported method: ") + conn.method);
        send_response(conn_id, conn, 405, "Method Not Allowed", "text/plain",
                      "405 Method Not Allowed");
    }
}

// -------------------------------------------------------------------------
// POST /mcp
// -------------------------------------------------------------------------
void HttpServer::handle_post(int conn_id, Connection &conn) {
    // ── /run-tests route (bypasses MCP entirely) ──
    if (conn.path == "/run-tests") {
        if (!test_engine_) {
            send_response(conn_id, conn, 503, "Service Unavailable", "text/plain",
                          "Test engine not initialized");
            return;
        }
        String body_text;
        if (!conn.body.is_empty()) {
            body_text.parse_utf8((const char *)conn.body.ptr(), conn.body.size());
        }
        if (body_text.is_empty()) {
            send_response(conn_id, conn, 400, "Bad Request", "text/plain", "Empty body");
            return;
        }
        const String json_result = handle_run_tests(body_text, *test_engine_);
        send_response(conn_id, conn, 200, "OK", "application/json; charset=utf-8", json_result);
        return;
    }

    if (conn.path != "/mcp") {
        send_response(conn_id, conn, 404, "Not Found", "text/plain", "404 Not Found");
        return;
    }

    // MCP-Protocol-Version: do NOT reject here. negotiate_protocol_version()
    // falls back to a supported version for unknown headers, and the actual
    // negotiation happens in handle_initialize using the body's protocolVersion
    // field. The previous strict pre-check rejected clients that sent a newer
    // header version even though the body would have negotiated fine.
    // (Header is still validated per-request by handle_message.)

    // Origin validation
    if (!validate_origin(conn)) {
        send_response(conn_id, conn, 403, "Forbidden", "text/plain", "Origin not allowed");
        return;
    }

    // Validate Accept header
    {
        auto accept_it = conn.headers.find("accept");
        const String accept = accept_it != conn.headers.end() ? accept_it->value : "";
        if (accept.find("application/json") == -1 && accept.find("*/*") == -1) {
            send_response(conn_id, conn, 406, "Not Acceptable", "text/plain",
                          "Client must accept application/json");
            return;
        }
    }

    // Validate Content-Type
    {
        auto ct_it = conn.headers.find("content-type");
        if (ct_it != conn.headers.end()) {
            const String ct = ct_it->value;
            if (ct.find("application/json") == -1) {
                send_response(conn_id, conn, 415, "Unsupported Media Type", "text/plain",
                              "Content-Type must be application/json");
                return;
            }
        }
    }

    // Body as text
    String body_text;
    if (!conn.body.is_empty()) {
        body_text.parse_utf8((const char *)conn.body.ptr(), conn.body.size());
    }

    if (body_text.is_empty()) {
        send_response(conn_id, conn, 400, "Bad Request", "text/plain", "Empty body");
        return;
    }

    // Parse JSON
    Ref<JSON> json;
    json.instantiate();
    const Error parse_err = json->parse(body_text);
    if (parse_err != OK) {
        Dictionary err_resp;
        err_resp["jsonrpc"] = "2.0";
        err_resp["id"] = Variant();
        Dictionary err;
        err["code"] = McpHandler::kParseError;
        err["message"] = json->get_error_message();
        err_resp["error"] = err;
        send_response(conn_id, conn, 200, "OK", "application/json; charset=utf-8", json_stringify_safe(err_resp));
        return;
    }

    const Variant root = json->get_data();

    // --- Batch ---
    if (root.get_type() == Variant::ARRAY) {
        const Array batch = root;
        Array responses;
        bool any_request = false;
        for (int i = 0; i < batch.size(); ++i) {
            // Per JSON-RPC 2.0 §6, a non-dictionary batch element is itself
            // invalid and must produce its own -32600 error response (the old
            // code silently `continue`d, yielding a misleading 202 for an
            // all-invalid batch).
            if (batch[i].get_type() != Variant::DICTIONARY) {
                Dictionary err_resp;
                Dictionary err;
                err["code"] = -32600; // Invalid Request
                err["message"] = "Invalid Request: batch element must be an object";
                err_resp["error"] = err;
                err_resp["id"] = Variant(); // null per spec for unidentifiable
                responses.push_back(err_resp);
                any_request = true;
                continue;
            }
            String batch_sid = conn.session_id;
            const Dictionary resp = mcp_handler_->handle_message(batch[i], batch_sid);
            if (!batch_sid.is_empty()) conn.session_id = batch_sid;
            if (!resp.is_empty()) {
                responses.push_back(resp);
                any_request = true;
            }
        }
        if (!any_request) {
            send_response(conn_id, conn, 202, "Accepted", "text/plain", "");
        } else {
            send_response(conn_id, conn, 200, "OK", "application/json; charset=utf-8",
                          json_stringify_safe(responses));
        }
        return;
    }

    // --- Single message ---
    if (root.get_type() != Variant::DICTIONARY) {
        send_response(conn_id, conn, 400, "Bad Request", "text/plain",
                      "Must be JSON object or array");
        return;
    }

    const Dictionary msg = root;

    // Notification or response (no `id` field or has `result`/`error`) �?202
    const bool has_result_or_error = msg.has("result") || msg.has("error");
    const bool is_notification_or_response = !msg.has("id") || has_result_or_error;

    if (is_notification_or_response) {
        String notify_sid = conn.session_id;
        mcp_handler_->handle_message(msg, notify_sid);
        if (!notify_sid.is_empty()) conn.session_id = notify_sid;
        send_response(conn_id, conn, 202, "Accepted", "text/plain", "");
        return;
    }

    // Process request via MCP handler
    String session_ref = conn.session_id;
    const Dictionary handler_result = mcp_handler_->handle_message(msg, session_ref);
    conn.session_id = session_ref;

    if (handler_result.is_empty()) {
        send_response(conn_id, conn, 202, "Accepted", "text/plain", "");
        return;
    }

    const String result_json = json_stringify_safe(handler_result);
    send_response(conn_id, conn, 200, "OK", "application/json; charset=utf-8", result_json);
}

// -------------------------------------------------------------------------
// GET /mcp �?SSE stream
// -------------------------------------------------------------------------
void HttpServer::handle_get(int conn_id, Connection &conn) {
    if (conn.path != "/mcp") {
        send_response(conn_id, conn, 404, "Not Found", "text/plain", "404 Not Found");
        return;
    }

    // Validate Accept header
    auto accept_it = conn.headers.find("accept");
    if (accept_it == conn.headers.end() ||
        accept_it->value.find("text/event-stream") == -1) {
        send_response(conn_id, conn, 405, "Method Not Allowed", "text/plain",
                      "GET only for text/event-stream");
        return;
    }

    if (!validate_origin(conn)) {
        send_response(conn_id, conn, 403, "Forbidden", "text/plain", "Origin not allowed");
        return;
    }

    if (conn.session_id.is_empty()) {
        send_response(conn_id, conn, 400, "Bad Request", "text/plain",
                      "MCP-Session-Id header required");
        return;
    }

    if (!mcp_handler_->validate_session(conn.session_id)) {
        send_response(conn_id, conn, 404, "Not Found", "text/plain", "Session not found");
        return;
    }

    // Reject duplicate SSE stream for the same session
    for (const auto &kv : connections_) {
        if (kv.key != conn_id && kv.value.is_sse_stream &&
            kv.value.session_id == conn.session_id) {
            send_response(conn_id, conn, 409, "Conflict", "text/plain",
                          "Only one SSE stream allowed per session");
            return;
        }
    }

    conn.is_sse_stream = true; // SSE resumption via Last-Event-ID is not supported
    conn.sse_last_event_msec = Time::get_singleton()->get_ticks_msec();
    send_sse_headers(conn_id, conn);
}

// -------------------------------------------------------------------------
// DELETE /mcp �?session termination
// -------------------------------------------------------------------------
void HttpServer::handle_delete(int conn_id, Connection &conn) {
    if (conn.path != "/mcp") {
        send_response(conn_id, conn, 404, "Not Found", "text/plain", "404 Not Found");
        return;
    }

    if (!conn.session_id.is_empty() && mcp_handler_->validate_session(conn.session_id)) {
        mcp_handler_->destroy_session(conn.session_id);
        send_response(conn_id, conn, 202, "Accepted", "text/plain", "");
    } else {
        log_warn("http", String("DELETE rejected for session=") + conn.session_id);
        send_response(conn_id, conn, 405, "Method Not Allowed", "text/plain",
                      "DELETE not available");
    }
}

void HttpServer::handle_options(int conn_id, Connection &conn) {
    String cors_headers = String("Access-Control-Allow-Origin: ") + get_cors_origin(conn) + String("\r\n") +
        "Vary: Origin\r\n"
        "Access-Control-Allow-Methods: GET, POST, DELETE, OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type, Accept, MCP-Session-Id, Last-Event-ID, MCP-Protocol-Version\r\n"
        "Access-Control-Max-Age: " + String::num_int64(kCorsMaxAgeSeconds) +
        "\r\nAccess-Control-Expose-Headers: MCP-Session-Id, Last-Event-ID, MCP-Protocol-Version\r\n";
    send_response(conn_id, conn, 204, "No Content", "text/plain", "", cors_headers);
}

} // namespace godot_mcp
