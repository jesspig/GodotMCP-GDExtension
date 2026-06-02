#include "http_server.hpp"
#include "test_http_handler.hpp"

#include "built_in/cmd_utils.hpp"
#include "logging.hpp"

#include <cstring>

#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/classes/stream_peer.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

namespace {

inline Error tcp_send(const Ref<StreamPeerTCP> &tcp, const PackedByteArray &data) {
    const Error err = tcp->put_data(data);
    tcp->poll();
    return err;
}

} // anonymous namespace

namespace godot_mcp {

HttpServer::HttpServer() = default;

HttpServer::~HttpServer() {
    stop();
}

bool HttpServer::start(int port, McpHandler *mcp_handler) {
    stop();
    mcp_handler_ = mcp_handler;
    port_ = port;

    tcp_server_.instantiate();
    const Error err = tcp_server_->listen((uint16_t)port, kBindAddress);
    if (err != OK) {
        log_error("http", String("Failed to listen on ") + String(kBindAddress) +
                              String(":") + String::num_int64(port) +
                              String(" (err=") + String::num_int64((int64_t)err) + String(")"));
        tcp_server_.unref();
        return false;
    }

    log_info("http",
             String("MCP Streamable HTTP on ") + String(kBindAddress) + String(":") + String::num_int64(port));
    return true;
}

void HttpServer::stop() {
    for (KeyValue<int, Connection> &kv : connections_) {
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

// -------------------------------------------------------------------------
// Poll
// -------------------------------------------------------------------------
void HttpServer::poll() {
    if (!tcp_server_.is_valid() || !tcp_server_->is_listening()) return;

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

    for (KeyValue<int, Connection> &kv : connections_) {
        const int conn_id = kv.key;
        Connection &conn = kv.value;
        if (conn.tcp.is_null()) { dead.push_back(conn_id); continue; }

        // SSE stream: flush events
        if (conn.is_sse_stream) {
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
                const Array read_result = conn.tcp->get_data((int)avail);
                if ((Error)(int)read_result[0] != OK) {
                    dead.push_back(conn_id); continue;
                }
                const PackedByteArray chunk = read_result[1];
                for (int i = 0; i < chunk.size(); ++i) conn.read_buf.push_back(chunk[i]);

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
                if (conn.content_length > 0 && conn.header_end_pos < conn.read_buf.size()) {
                    for (int i = conn.header_end_pos; i < conn.read_buf.size(); ++i) {
                        conn.body.push_back(conn.read_buf[i]);
                    }
                }
            } else {
                // Headers already done: read body data directly from socket
                if (conn.content_length > 0 && conn.body.size() < conn.content_length) {
                    const int remaining = conn.content_length - conn.body.size();
                    const int max_read = kMaxBodyLength - conn.body.size();
                    int to_read = (int)avail < remaining ? (int)avail : remaining;
                    if (to_read > max_read) to_read = max_read;
                    if (to_read <= 0) { dead.push_back(conn_id); continue; }
                    const Array read_result = conn.tcp->get_data(to_read);
                    if ((Error)(int)read_result[0] != OK) {
                        dead.push_back(conn_id); continue;
                    }
                    const PackedByteArray read_chunk = read_result[1];
                    for (int i = 0; i < read_chunk.size(); ++i) conn.body.push_back(read_chunk[i]);
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
                if (conn.content_length > 0 && conn.header_end_pos < conn.read_buf.size()) {
                    for (int i = conn.header_end_pos; i < conn.read_buf.size(); ++i) {
                        conn.body.push_back(conn.read_buf[i]);
                    }
                }
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

        // Body complete → dispatch
        if (conn.headers_done && (conn.content_length <= 0 || conn.body.size() >= conn.content_length)) {
            // Non-SSE connections close after each request (poll architecture can't
            // reliably handle rapid-fire keepalive due to frame timing).
            if (!conn.is_sse_stream) {
                conn.keep_alive = false;
            }
            dispatch_request(conn_id, conn);
            if (!conn.is_sse_stream && connections_.has(conn_id)) {
                close_connection(conn_id);
            }
        }
    }

    for (int i = 0; i < dead.size(); ++i) close_connection(dead[i]);
}

// -------------------------------------------------------------------------
// HTTP header parsing
// -------------------------------------------------------------------------
HttpServer::ParseResult HttpServer::parse_headers(Connection &conn) {
    const Vector<uint8_t> &buf = conn.read_buf;

    // Find \r\n\r\n
    int header_end = -1;
    for (int i = 0; i < buf.size() - 3; ++i) {
        if (buf[i] == '\r' && buf[i + 1] == '\n' &&
            buf[i + 2] == '\r' && buf[i + 3] == '\n') {
            header_end = i;
            break;
        }
    }
    if (header_end < 0) return NEED_MORE;

    conn.header_end_pos = header_end + 4;

    String header_section;
    header_section.parse_utf8((const char *)buf.ptr(), header_end);

    // Status line
    const int first_lf = header_section.find("\r\n");
    if (first_lf < 0) return ERROR_PARSE;
    const String status_line = header_section.substr(0, first_lf);

    const int sp1 = status_line.find(" ");
    if (sp1 < 0) return ERROR_PARSE;
    conn.method = status_line.substr(0, sp1);

    const int sp2 = status_line.find(" ", sp1 + 1);
    if (sp2 < 0) return ERROR_PARSE;
    conn.path = status_line.substr(sp1 + 1, sp2 - sp1 - 1);

    // Parse headers (max kMaxHeaders)
    int line_start = first_lf + 2;
    int header_count = 0;
    while (line_start < header_end) {
        const int line_end = header_section.find("\r\n", line_start);
        if (line_end < 0) break;
        const String line = header_section.substr(line_start, line_end - line_start);
        const int colon = line.find(":");
        if (colon > 0) {
            if (++header_count > kMaxHeaders) {
                log_warn("http", "Too many headers (> " + String::num_int64(kMaxHeaders) + ")");
                return ERROR_PARSE;
            }
            String key = line.substr(0, colon).to_lower().strip_edges();
            String value = line.substr(colon + 1).strip_edges();
            conn.headers[key] = value;
        }
        line_start = line_end + 2;
    }

    // Content-Length
    auto cl_it = conn.headers.find("content-length");
    if (cl_it != conn.headers.end()) {
        conn.content_length = (int)cl_it->value.to_int();
        if (conn.content_length > kMaxBodyLength) {
            log_warn("http", String("Content-Length ") + String::num_int64(conn.content_length) +
                                 String(" exceeds max ") + String::num_int64(kMaxBodyLength));
            return ERROR_PARSE;
        }
    }

    // If no Content-Length but body data exists after headers, infer length from buffer
    if (conn.content_length <= 0 && conn.header_end_pos < conn.read_buf.size()) {
        conn.content_length = conn.read_buf.size() - conn.header_end_pos;
        if (conn.content_length > kMaxBodyLength) {
            conn.content_length = kMaxBodyLength;
        }
    }

    // Connection
    auto c_it = conn.headers.find("connection");
    if (c_it != conn.headers.end() && c_it->value.to_lower().find("close") != -1) {
        conn.keep_alive = false;
    }

    // MCP-Session-Id
    auto sid_it = conn.headers.find("mcp-session-id");
    if (sid_it != conn.headers.end()) {
        conn.session_id = sid_it->value;
    }

    conn.headers_done = true;
    return COMPLETE;
}

void HttpServer::try_read_body(Connection &conn) {
    if (conn.content_length <= 0) return;
    const int remaining = conn.content_length - conn.body.size();
    if (remaining <= 0) return;

    if (conn.body.size() >= kMaxBodyLength) return;

    conn.tcp->poll();
    const int64_t avail = conn.tcp->get_available_bytes();
    if (avail <= 0) return;

    const int max_read = kMaxBodyLength - conn.body.size();
    const int to_read = (int)avail < remaining ? (int)avail : remaining;
    const int safe_read = to_read < max_read ? to_read : max_read;
    if (safe_read <= 0) return;

    const Array read_result = conn.tcp->get_data(safe_read);
    if ((Error)(int)read_result[0] != OK) return;

    const PackedByteArray chunk = read_result[1];
    for (int i = 0; i < chunk.size(); ++i) conn.body.push_back(chunk[i]);
    conn.last_activity_msec = Time::get_singleton()->get_ticks_msec();
}

// -------------------------------------------------------------------------
// Request dispatch
// -------------------------------------------------------------------------
void HttpServer::dispatch_request(int conn_id, Connection &conn) {
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

    // MCP-Protocol-Version validation
    auto pv_it = conn.headers.find("mcp-protocol-version");
    if (pv_it != conn.headers.end()) {
        const String negotiated = McpHandler::negotiate_protocol_version(pv_it->value);
        if (negotiated != pv_it->value) {
            send_response(conn_id, conn, 400, "Bad Request", "text/plain",
                          "Unsupported protocol version. Supported: 2025-03-26, 2025-11-25");
            return;
        }
    }

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
        err["code"] = -32700;
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
            if (batch[i].get_type() != Variant::DICTIONARY) continue;
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

    // Notification or response (no `id` field or has `result`/`error`) → 202
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
// GET /mcp — SSE stream
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
    for (const KeyValue<int, Connection> &kv : connections_) {
        if (kv.key != conn_id && kv.value.is_sse_stream &&
            kv.value.session_id == conn.session_id) {
            send_response(conn_id, conn, 409, "Conflict", "text/plain",
                          "Only one SSE stream allowed per session");
            return;
        }
    }

    // Handle SSE resumption via Last-Event-ID
    auto lei_it = conn.headers.find("last-event-id");
    if (lei_it != conn.headers.end()) {
        conn.sse_event_id = (int)lei_it->value.to_int();
    }

    conn.is_sse_stream = true;
    send_sse_headers(conn_id, conn);
}

// -------------------------------------------------------------------------
// DELETE /mcp — session termination
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
    String cors_headers = String("Access-Control-Allow-Origin: *\r\n") +
        "Access-Control-Allow-Methods: GET, POST, DELETE, OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type, Accept, MCP-Session-Id, Last-Event-ID, MCP-Protocol-Version\r\n"
        "Access-Control-Max-Age: " + String::num_int64(kCorsMaxAgeSeconds) +
        "\r\nAccess-Control-Expose-Headers: MCP-Session-Id, Last-Event-ID, MCP-Protocol-Version\r\n";
    send_response(conn_id, conn, 204, "No Content", "text/plain", "", cors_headers);
}

// -------------------------------------------------------------------------
// Response helpers
// -------------------------------------------------------------------------
void HttpServer::send_response(int conn_id, Connection &conn, int status_code,
                                const String &status_text, const String &content_type,
                                const String &body, const String &extra_headers) {
    String response = String("HTTP/1.1 ") + String::num_int64(status_code) +
                      String(" ") + status_text + String("\r\n");

    const PackedByteArray body_bytes = body.to_utf8_buffer();

    response += String("Content-Type: ") + content_type + String("\r\n");
    response += String("Content-Length: ") + String::num_int64(body_bytes.size()) + String("\r\n");

    if (conn.keep_alive && !conn.is_sse_stream) {
        response += "Connection: keep-alive\r\n";
    } else {
        response += "Connection: close\r\n";
    }

    response += "Cache-Control: no-store\r\n";
    response += "Access-Control-Allow-Origin: *\r\n";
    response += "Access-Control-Expose-Headers: MCP-Session-Id, Last-Event-ID, MCP-Protocol-Version\r\n";

    if (!conn.session_id.is_empty()) {
        response += String("MCP-Session-Id: ") + conn.session_id + String("\r\n");
    }

    if (!extra_headers.is_empty()) {
        response += extra_headers;
        if (!extra_headers.ends_with("\r\n")) response += "\r\n";
    }

    response += "\r\n";
    response += body;

    const PackedByteArray out = response.to_utf8_buffer();
    if (conn.tcp.is_valid()) {
        conn.tcp->poll();
        const Error send_err = tcp_send(conn.tcp, out);
        if (send_err != OK) {
            log_warn("http", String("Send failed (err=") + String::num_int64((int64_t)send_err) + String(")"));
        }
    }
}

void HttpServer::send_sse_headers(int conn_id, Connection &conn) {
    String response = String("HTTP/1.1 200 OK\r\n") +
                      String("Content-Type: text/event-stream; charset=utf-8\r\n") +
                      String("Cache-Control: no-cache\r\n") +
                      String("Connection: keep-alive\r\n") +
                      String("X-Accel-Buffering: no\r\n") +
                      String("Access-Control-Allow-Origin: *\r\n");

    if (!conn.session_id.is_empty()) {
        response += String("MCP-Session-Id: ") + conn.session_id + String("\r\n");
    }

    response += String("\r\n");

    const PackedByteArray out = response.to_utf8_buffer();
    if (conn.tcp.is_valid()) {
        conn.tcp->poll();
        const Error err = tcp_send(conn.tcp, out);
        if (err != OK) {
            log_warn("http", String("SSE header send failed (err=") + String::num_int64((int64_t)err) + String(")"));
            conn.sse_write_errored = true;
            return;
        }
    }

    send_sse_comment(conn_id, conn, "retry:5000");
}

void HttpServer::send_sse_event(int conn_id, Connection &conn,
                                 const String &event_type, const String &data, int id) {
    String event;
    if (id > 0) event += String("id: ") + String::num_int64(id) + String("\r\n");
    event += String("event: ") + event_type + String("\r\n");
    event += String("data: ") + data + String("\r\n");
    event += String("\r\n");

    const PackedByteArray out = event.to_utf8_buffer();
    if (conn.tcp.is_valid()) {
        conn.tcp->poll();
        const Error err = tcp_send(conn.tcp, out);
        if (err != OK) {
            log_warn("http", String("SSE send event failed (err=") + String::num_int64((int64_t)err) + String(")"));
            conn.sse_write_errored = true;
        }
    }
}

void HttpServer::send_sse_comment(int conn_id, Connection &conn, const String &comment) {
    const String msg = String(": ") + comment + String("\r\n\r\n");
    const PackedByteArray out = msg.to_utf8_buffer();
    if (conn.tcp.is_valid()) {
        conn.tcp->poll();
        const Error err = tcp_send(conn.tcp, out);
        if (err != OK) {
            log_warn("http", String("SSE send comment failed (err=") + String::num_int64((int64_t)err) + String(")"));
            conn.sse_write_errored = true;
        }
    }
}

void HttpServer::flush_sse(int conn_id, Connection &conn) {
    if (!mcp_handler_ || conn.session_id.is_empty()) return;

    const uint64_t now = Time::get_singleton()->get_ticks_msec();

    // Keep-alive if idle
    if (!mcp_handler_->has_pending_events(conn.session_id)) {
        if (now - conn.last_activity_msec > 15000) {
            send_sse_comment(conn_id, conn, "keep-alive");
            conn.last_activity_msec = now;
        }
        return;
    }

    while (mcp_handler_->has_pending_events(conn.session_id)) {
        if (conn.sse_write_errored) return;

        Dictionary event = mcp_handler_->consume_event(conn.session_id);
        const String method = event.get("method", "");
        const Variant params = event.get("params", Variant());

        String data;
        if (params.get_type() == Variant::DICTIONARY) {
            Dictionary payload;
            payload["jsonrpc"] = "2.0";
            payload["method"] = method;
            payload["params"] = params;
            data = json_stringify_safe(payload);
        } else {
            data = json_stringify_safe(event);
        }

        conn.sse_event_id++;
        send_sse_event(conn_id, conn, "message", data, conn.sse_event_id);
        conn.last_activity_msec = now;
    }
}

// -------------------------------------------------------------------------
// Connection management
// -------------------------------------------------------------------------
void HttpServer::close_connection(int conn_id) {
    auto it = connections_.find(conn_id);
    if (it == connections_.end()) return;
    Connection &conn = it->value;
    if (conn.tcp.is_valid()) conn.tcp->disconnect_from_host();
    connections_.erase(it->key);
}

void HttpServer::check_timeouts() {
    const uint64_t now = Time::get_singleton()->get_ticks_msec();
    Vector<int> timed_out;
    for (KeyValue<int, Connection> &kv : connections_) {
        if (now - kv.value.last_activity_msec > timeout_msec_) {
            timed_out.push_back(kv.key);
        }
    }
    for (int i = 0; i < timed_out.size(); ++i) {
        close_connection(timed_out[i]);
    }
}

bool HttpServer::validate_origin(const Connection &conn) const {
    auto it = conn.headers.find("origin");
    if (it == conn.headers.end()) return true;
    const String origin = it->value;
    if (origin.find("127.0.0.1") != -1 ||
        origin.find("localhost") != -1 ||
        origin.find("null") != -1) {
        return true;
    }
    log_warn("http", String("Blocked origin: ") + origin);
    return false;
}

} // namespace godot_mcp
