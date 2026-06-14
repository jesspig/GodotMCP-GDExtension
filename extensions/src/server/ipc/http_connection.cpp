#include "http_server.hpp"
#include "logging.hpp"

#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/classes/stream_peer.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

namespace godot_mcp {

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
    response += String("Access-Control-Allow-Origin: ") + get_cors_origin(conn) + String("\r\n");
    response += "Vary: Origin\r\n";
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

bool HttpServer::is_local_origin(const String &origin) {
    const PackedStringArray parts = origin.split("://");
    String host;
    if (parts.size() == 2) {
        host = parts[1].split(":")[0];
    } else {
        host = parts[0].split(":")[0];
    }
    return host == "localhost" || host == "127.0.0.1" || host == "::1" || origin == "null";
}

bool HttpServer::validate_origin(const Connection &conn) const {
    auto it = conn.headers.find("origin");
    if (it == conn.headers.end()) return true;
    const String origin = it->value.strip_edges();
    if (origin.is_empty()) return true;
    if (is_local_origin(origin)) return true;
    log_warn("http", String("Blocked origin: ") + origin);
    return false;
}

String HttpServer::get_cors_origin(const Connection &conn) const {
    auto it = conn.headers.find("origin");
    if (it == conn.headers.end()) return "*";
    const String origin = it->value.strip_edges();
    if (origin.is_empty()) return "*";
    return is_local_origin(origin) ? origin : String("null");
}

} // namespace godot_mcp
