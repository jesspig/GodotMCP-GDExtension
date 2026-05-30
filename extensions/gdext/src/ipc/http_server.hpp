#pragma once

#include "../mcp/mcp_handler.hpp"

#include <godot_cpp/classes/tcp_server.hpp>
#include <godot_cpp/classes/stream_peer_tcp.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot_mcp {

class HttpServer {
public:
    HttpServer();
    ~HttpServer();

    bool start(int port, McpHandler *mcp_handler);
    void stop();
    void poll();
    bool is_listening() const;

    static constexpr int kMaxConnections = 32;
    static constexpr int kMaxBodyLength = 1048576; // 1 MB — prevent OOM on oversized payloads
    static constexpr int kMaxHeaders = 100;
    static constexpr const char *kBindAddress = "127.0.0.1";
    static constexpr int kCorsMaxAgeSeconds = 86400;

private:
    struct Connection {
        godot::Ref<godot::StreamPeerTCP> tcp;
        godot::Vector<uint8_t> read_buf;
        uint64_t last_activity_msec = 0;
        godot::String session_id;

        bool headers_done = false;
        godot::String method;
        godot::String path;
        godot::HashMap<godot::String, godot::String> headers;
        godot::Vector<uint8_t> body;
        int content_length = 0;
        int header_end_pos = -1;

        bool is_sse_stream = false;
        bool sse_write_errored = false;
        int sse_event_id = 0;
        bool keep_alive = true;
    };

    enum ParseResult { NEED_MORE, COMPLETE, ERROR_PARSE };

    ParseResult parse_headers(Connection &conn);
    void try_read_body(Connection &conn);

    void dispatch_request(int conn_id, Connection &conn);
    void handle_post(int conn_id, Connection &conn);
    void handle_get(int conn_id, Connection &conn);
    void handle_delete(int conn_id, Connection &conn);
    void handle_options(int conn_id, Connection &conn);

    void send_response(int conn_id, Connection &conn, int status_code,
                       const godot::String &status_text, const godot::String &content_type,
                       const godot::String &body,
                       const godot::String &extra_headers = "");
    void send_sse_headers(int conn_id, Connection &conn);
    void send_sse_event(int conn_id, Connection &conn, const godot::String &event_type,
                        const godot::String &data, int id = 0);
    void send_sse_comment(int conn_id, Connection &conn, const godot::String &comment);
    void flush_sse(int conn_id, Connection &conn);

    void close_connection(int conn_id);
    void check_timeouts();
    bool validate_origin(const Connection &conn) const;

    godot::Ref<godot::TCPServer> tcp_server_;
    godot::HashMap<int, Connection> connections_;
    int next_conn_id_ = 0;
    McpHandler *mcp_handler_ = nullptr;
    int port_ = 0;
    uint64_t timeout_msec_ = 30000;

};

} // namespace godot_mcp
