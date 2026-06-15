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

    response += String("\r\n");

    const PackedByteArray out = response.to_utf8_buffer();
    if (conn.tcp.is_valid()) {
        conn.tcp->poll();
        const Error err = tcp_send(conn.tcp, out);
        if (err != OK) {
            log_warn("http", String("SSE header send failed (err=") + String::num_int64(static_cast<int64_t>(err)) + String(")"));
            conn.sse_write_errored = true;
            return;
        }
    }

    const String retry_line = String("retry: ") + String::num_int64(kSseRetryIntervalMsec) + String("\r\n");
    const PackedByteArray retry_out = retry_line.to_utf8_buffer();
    if (conn.tcp.is_valid()) {
        conn.tcp->poll();
        const Error err = tcp_send(conn.tcp, retry_out);
        if (err != OK) {
            log_warn("http", String("SSE retry send failed (err=") + String::num_int64(static_cast<int64_t>(err)) + String(")"));
            conn.sse_write_errored = true;
            return;
        }
    }
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
            log_warn("http", String("SSE send event failed (err=") + String::num_int64(static_cast<int64_t>(err)) + String(")"));
            conn.sse_write_errored = true;
        }
    }
}

void HttpServer::send_sse_comment(int conn_id, Connection &conn, const String &comment) {
    const String msg = String(": ") + comment + String("\r\n");
    const PackedByteArray out = msg.to_utf8_buffer();
    if (conn.tcp.is_valid()) {
        conn.tcp->poll();
        const Error err = tcp_send(conn.tcp, out);
        if (err != OK) {
            log_warn("http", String("SSE send comment failed (err=") + String::num_int64(static_cast<int64_t>(err)) + String(")"));
            conn.sse_write_errored = true;
        }
    }
}

void HttpServer::flush_sse(int conn_id, Connection &conn) {
    if (!mcp_handler_) return;

    while (mcp_handler_->has_pending_events()) {
        if (conn.sse_write_errored) return;

        Dictionary event = mcp_handler_->consume_event();
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

        // Event IDs are sent for SSE spec compliance; resumption via Last-Event-ID is not supported
        conn.sse_event_id++;
        send_sse_event(conn_id, conn, "message", data, conn.sse_event_id);
    }
}

} // namespace godot_mcp
