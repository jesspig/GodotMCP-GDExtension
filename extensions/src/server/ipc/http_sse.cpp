#include "http_server.hpp"
#include "built_in/cmd_utils.hpp"
#include "logging.hpp"

#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/classes/stream_peer.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

namespace godot_mcp {

void HttpServer::send_sse_headers(int conn_id, Connection &conn) {
    String response = String("HTTP/1.1 200 OK\r\n") +
                      String("Content-Type: text/event-stream; charset=utf-8\r\n") +
                      String("Cache-Control: no-cache\r\n") +
                      String("Connection: keep-alive\r\n") +
                      String("X-Accel-Buffering: no\r\n") +
                      String("Access-Control-Allow-Origin: ") + get_cors_origin(conn) + String("\r\n") +
                      String("Vary: Origin\r\n");

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

} // namespace godot_mcp
