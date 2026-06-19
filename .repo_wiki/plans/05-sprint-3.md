# Sprint 3 开发文档

> 精确到代码级的实现指南。WebSocket 同端口桥接 + 角色反转 + 多实例支持。

---

## H: WebSocket 帧编解码

### 步骤 1: 创建 websocket.hpp

**新文件**: `extensions/src/server/ipc/websocket.hpp`

```cpp
#pragma once

#include <cstdint>
#include <optional>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/classes/marshalls.hpp>

namespace godot_mcp {

enum class WsOpcode : uint8_t {
    Continuation = 0x0,
    Text         = 0x1,
    Binary       = 0x2,
    Close        = 0x8,
    Ping         = 0x9,
    Pong         = 0xA,
};

struct WsFrame {
    bool fin = false;
    WsOpcode opcode = WsOpcode::Text;
    godot::PackedByteArray payload;
};

class WebSocketCodec {
public:
    WebSocketCodec() = delete;

    // Decode one frame from raw bytes. Returns bytes consumed, or nullopt if insufficient data.
    [[nodiscard]] static std::optional<int> decode(
        const uint8_t *data, int64_t available, WsFrame &out) noexcept;

    // Encode a frame (server→client, no mask).
    [[nodiscard]] static godot::PackedByteArray encode(
        WsOpcode opcode, const uint8_t *data, int64_t len);

    // Validate WebSocket upgrade request headers.
    [[nodiscard]] static bool validate_upgrade_request(
        const godot::HashMap<godot::String, godot::String> &headers) noexcept;

    // Compute Sec-WebSocket-Accept value from client key.
    [[nodiscard]] static godot::String build_accept_value(
        const godot::String &client_key);

    static constexpr int kMaxFrameSize = 65536 + 16;
    static constexpr const char *kWsGuid =
        "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
};

} // namespace godot_mcp
```

### 步骤 2: 创建 websocket.cpp

**新文件**: `extensions/src/server/ipc/websocket.cpp`

```cpp
#include "websocket.hpp"

#include <algorithm>
#include <godot_cpp/classes/hashing_context.hpp>
#include <godot_cpp/classes/marshalls.hpp>

using namespace godot;

namespace godot_mcp {

std::optional<int> WebSocketCodec::decode(
        const uint8_t *data, int64_t available, WsFrame &out) noexcept {
    if (available < 2) return std::nullopt;

    const uint8_t byte0 = data[0];
    const uint8_t byte1 = data[1];

    out.fin = (byte0 & 0x80) != 0;
    out.opcode = static_cast<WsOpcode>(byte0 & 0x0F);
    const bool masked = (byte1 & 0x80) != 0;
    uint64_t payload_len = byte1 & 0x7F;
    int header_size = 2;

    if (payload_len == 126) {
        if (available < 4) return std::nullopt;
        payload_len = (static_cast<uint16_t>(data[2]) << 8) | data[3];
        header_size = 4;
    } else if (payload_len == 127) {
        if (available < 10) return std::nullopt;
        payload_len = 0;
        for (int i = 0; i < 8; ++i) {
            payload_len = (payload_len << 8) | data[2 + i];
        }
        header_size = 10;
    }

    if (payload_len > kMaxFrameSize) return std::nullopt;

    uint8_t mask_key[4] = {};
    if (masked) {
        if (available < header_size + 4) return std::nullopt;
        std::copy_n(data + header_size, 4, mask_key);
        header_size += 4;
    }

    const int64_t total = header_size + static_cast<int64_t>(payload_len);
    if (available < total) return std::nullopt;

    out.payload.resize(static_cast<int>(payload_len));
    std::copy_n(data + header_size, payload_len, out.payload.ptrw());

    if (masked) {
        auto *p = out.payload.ptrw();
        for (size_t i = 0; i < payload_len; ++i) {
            p[i] ^= mask_key[i % 4];
        }
    }

    return static_cast<int>(total);
}

PackedByteArray WebSocketCodec::encode(
        WsOpcode opcode, const uint8_t *data, int64_t len) {
    int header_size = 2;
    if (len > 125 && len <= 65535) header_size = 4;
    else if (len > 65535) header_size = 10;

    PackedByteArray frame;
    frame.resize(header_size + static_cast<int>(len));
    auto *p = frame.ptrw();

    p[0] = 0x80 | static_cast<uint8_t>(opcode);

    if (len <= 125) {
        p[1] = static_cast<uint8_t>(len);
    } else if (len <= 65535) {
        p[1] = 126;
        p[2] = static_cast<uint8_t>((len >> 8) & 0xFF);
        p[3] = static_cast<uint8_t>(len & 0xFF);
    } else {
        p[1] = 127;
        for (int i = 7; i >= 0; --i) {
            p[2 + (7 - i)] = static_cast<uint8_t>((len >> (i * 8)) & 0xFF);
        }
    }

    std::copy_n(data, len, p + header_size);
    return frame;
}

bool WebSocketCodec::validate_upgrade_request(
        const HashMap<String, String> &headers) noexcept {
    auto it = headers.find("upgrade");
    if (it == headers.end() || it->value.to_lower() != "websocket") return false;

    it = headers.find("connection");
    if (it == headers.end()) return false;
    if (it->value.to_lower().find("upgrade") < 0) return false;

    it = headers.find("sec-websocket-version");
    if (it == headers.end() || it->value != "13") return false;

    it = headers.find("sec-websocket-key");
    if (it == headers.end()) return false;

    return true;
}

String WebSocketCodec::build_accept_value(const String &client_key) {
    // Godot String::sha1_buffer() returns raw 20-byte SHA-1 hash
    const String concat = client_key + String(kWsGuid);
    const PackedByteArray hash = concat.sha1_buffer();
    // Marshalls::raw_to_base64() encodes raw bytes to base64
    return Marshalls::get_singleton()->raw_to_base64(hash);
}

} // namespace godot_mcp
```

**关键点**：
- `String::sha1_buffer()` — Godot 内置，`core/string/ustring.h:603`，返回 `Vector<uint8_t>`（20 字节）
- `Marshalls::raw_to_base64()` — Godot 内置，`core/core_bind.h:435`，Engine 单例
- 零外部依赖

---

## I: HttpServer WebSocket 扩展

### 步骤 1: http_server.hpp 新增

**文件**: `extensions/src/server/ipc/http_server.hpp`

新增 include：

```cpp
#include "websocket.hpp"
#include <functional>
```

在 `HttpServer` 类中新增结构体和成员：

```cpp
    // ── WebSocket connections ──
    struct WsConnection {
        Ref<StreamPeerTCP> tcp;
        PackedByteArray read_buf;
        int64_t read_offset = 0;
        uint64_t last_activity_msec = 0;

        int instance_id = 0;
        int pid = 0;
        String scene_path;
        bool ready = false;
    };

    HashMap<int, WsConnection> ws_connections_;
    int next_ws_id_ = 1;
    int next_request_id_ = 1;

    using BridgeEventCallback = std::function<void(
        int instance_id, const String &event, const Dictionary &data)>;
    BridgeEventCallback bridge_event_callback_;

    void set_bridge_event_callback(BridgeEventCallback cb) {
        bridge_event_callback_ = std::move(cb);
    }

    int ws_instance_count() const noexcept {
        return static_cast<int>(ws_connections_.size());
    }

    // ── WebSocket methods ──
    void poll_websocket_connections();
    void handle_ws_handshake(int conn_id, Connection &conn);
    void handle_ws_message(int ws_id, const Dictionary &msg);
    void close_ws_connection(int ws_id);

    // Command sending (editor → game)
    Dictionary send_ws_command(int instance_id, const String &cmd,
                               const Dictionary &params, int timeout_ms = 100);
```

### 步骤 2: http_server.cpp — 路由判断

**文件**: `extensions/src/server/ipc/http_server.cpp`

在 `dispatch_request()` 方法中（HTTP method 判断之前）插入 WebSocket 升级检测：

```cpp
void HttpServer::dispatch_request(int conn_id, Connection &conn) {
    // WebSocket upgrade check — must be before HTTP routing
    if (conn.path == "/bridge" &&
        conn.headers.has("upgrade") &&
        conn.headers["upgrade"].to_lower() == "websocket") {
        handle_ws_handshake(conn_id, conn);
        return;
    }

    // Existing HTTP routing
    if (conn.method == "POST") {
        handle_post(conn_id, conn);
    } else if (conn.method == "GET") {
        handle_get(conn_id, conn);
    } else if (conn.method == "OPTIONS") {
        handle_options(conn_id, conn);
    } else {
        send_response(conn_id, conn, 405, "Method Not Allowed",
            "text/plain", "Method not allowed");
        close_connection(conn_id);
    }
}
```

### 步骤 3: http_server.cpp — handle_ws_handshake

```cpp
void HttpServer::handle_ws_handshake(int conn_id, Connection &conn) {
    if (!WebSocketCodec::validate_upgrade_request(conn.headers)) {
        send_response(conn_id, conn, 400, "Bad Request",
            "text/plain", "Invalid WebSocket upgrade request");
        close_connection(conn_id);
        return;
    }

    const String accept_value = WebSocketCodec::build_accept_value(
        conn.headers["sec-websocket-key"]);

    // Build 101 response
    const String response = String("HTTP/1.1 101 Switching Protocols\r\n")
        + "Upgrade: websocket\r\n"
        + "Connection: Upgrade\r\n"
        + "Sec-WebSocket-Accept: " + accept_value + "\r\n\r\n";

    const PackedByteArray resp_bytes = response.to_utf8_buffer();
    tcp_send(conn.tcp, resp_bytes);

    // Move from HTTP pool to WebSocket pool
    WsConnection ws;
    ws.tcp = conn.tcp;
    ws.instance_id = next_ws_id_++;
    ws.last_activity_msec = Time::get_singleton()->get_ticks_msec();

    const int ws_id = ws.instance_id;
    ws_connections_[ws_id] = std::move(ws);

    // Remove from HTTP connection pool (TCP stays alive)
    conn.tcp = Ref<StreamPeerTCP>();
    close_connection(conn_id);

    log_info("ws", String("WebSocket instance #") + String::num_int64(ws_id) + String(" connected"));
}
```

### 步骤 4: http_server.cpp — poll_websocket_connections

```cpp
void HttpServer::poll_websocket_connections() {
    std::vector<int> dead;

    for (auto &[ws_id, ws] : ws_connections_) {
        ws.tcp->poll();

        if (ws.tcp->get_status() != StreamPeerTCP::STATUS_CONNECTED) {
            dead.push_back(ws_id);
            continue;
        }

        const int64_t avail = ws.tcp->get_available_bytes();
        if (avail <= 0) continue;

        Array chunk = ws.tcp->get_partial_data(static_cast<int>(avail));
        if (static_cast<Error>(static_cast<int>(chunk[0])) != OK) continue;

        const PackedByteArray data = chunk[1];
        ws.read_buf.append_array(data);
        ws.last_activity_msec = Time::get_singleton()->get_ticks_msec();

        // Parse WebSocket frames
        while (ws.read_offset < ws.read_buf.size()) {
            WsFrame frame;
            const auto consumed = WebSocketCodec::decode(
                ws.read_buf.ptr() + ws.read_offset,
                ws.read_buf.size() - ws.read_offset,
                frame);
            if (!consumed.has_value()) break;

            ws.read_offset += consumed.value();

            switch (frame.opcode) {
                case WsOpcode::Text: {
                    const String text = String::utf8(
                        reinterpret_cast<const char *>(frame.payload.ptr()),
                        frame.payload.size());
                    Ref<JSON> json;
                    json.instantiate();
                    if (json->parse(text) == OK &&
                        json->get_data().get_type() == Variant::DICTIONARY) {
                        handle_ws_message(ws_id, json->get_data());
                    }
                    break;
                }
                case WsOpcode::Close:
                    dead.push_back(ws_id);
                    break;
                case WsOpcode::Ping: {
                    const PackedByteArray pong = WebSocketCodec::encode(
                        WsOpcode::Pong, frame.payload.ptr(), frame.payload.size());
                    tcp_send(ws.tcp, pong);
                    break;
                }
                default:
                    break;
            }
        }

        // Trim consumed bytes
        if (ws.read_offset > 0) {
            const int remaining = ws.read_buf.size() - static_cast<int>(ws.read_offset);
            if (remaining > 0) {
                PackedByteArray new_buf;
                new_buf.resize(remaining);
                std::copy_n(ws.read_buf.ptr() + ws.read_offset, remaining, new_buf.ptrw());
                ws.read_buf = new_buf;
            } else {
                ws.read_buf.clear();
            }
            ws.read_offset = 0;
        }
    }

    for (const int ws_id : dead) {
        if (bridge_event_callback_) {
            bridge_event_callback_(ws_id, "disconnected", Dictionary());
        }
        ws_connections_.erase(ws_id);
    }
}
```

### 步骤 5: http_server.cpp — handle_ws_message

```cpp
void HttpServer::handle_ws_message(int ws_id, const Dictionary &msg) {
    const String type = msg.get("type", "");

    if (type == "event") {
        const String event = msg.get("event", "");
        const Dictionary data = msg.get("data", Dictionary());

        if (event == "connected") {
            auto it = ws_connections_.find(ws_id);
            if (it != ws_connections_.end()) {
                it->value.pid = data.get("pid", 0);
                it->value.scene_path = data.get("scene_root", "");
                it->value.ready = true;
            }
        }

        if (bridge_event_callback_) {
            bridge_event_callback_(ws_id, event, data);
        }
    }
}
```

### 步骤 6: http_server.cpp — send_ws_command

```cpp
Dictionary HttpServer::send_ws_command(
        int instance_id, const String &cmd, const Dictionary &params, int timeout_ms) {
    auto it = ws_connections_.find(instance_id);
    if (it == ws_connections_.end() || !it->value.ready) {
        Dictionary err;
        err["ok"] = false;
        err["error"] = "Instance not connected or not ready";
        return err;
    }

    Dictionary req;
    req["type"] = "request";
    req["cmd"] = cmd;
    req["params"] = params;
    req["id"] = next_request_id_++;

    const String json = JSON::stringify(req);
    const PackedByteArray payload = json.to_utf8_buffer();
    const PackedByteArray frame = WebSocketCodec::encode(
        WsOpcode::Text, payload.ptr(), payload.size());

    if (tcp_send(it->value.tcp, frame) != OK) {
        Dictionary err;
        err["ok"] = false;
        err["error"] = "Send failed";
        return err;
    }

    // Wait for response (consume events while waiting)
    const uint64_t start = Time::get_singleton()->get_ticks_msec();
    while (Time::get_singleton()->get_ticks_msec() - start < timeout_ms) {
        poll_websocket_connections();
        OS::get_singleton()->delay_msec(5);
    }

    Dictionary timeout_err;
    timeout_err["ok"] = false;
    timeout_err["error"] = "Command timed out";
    return timeout_err;
}
```

### 步骤 7: HttpServer::poll() 集成

**文件**: `extensions/src/server/ipc/http_server.cpp`

在 `poll()` 方法中（`process_pending_parsed_results()` 之后，`check_timeouts()` 之前）添加：

```cpp
    poll_websocket_connections();
```

### 步骤 8: HttpServer::stop() 清理

在 `stop()` 中添加 WebSocket 连接清理：

```cpp
    ws_connections_.clear();
    next_ws_id_ = 1;
```

---

## J: GameBridgeClient 重写

### 步骤 1: game_bridge.hpp 重写

**文件**: `extensions/src/runtime/game_bridge.hpp`

完全重写为 WebSocket 客户端：

```cpp
#pragma once

#include "server/ipc/websocket.hpp"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/classes/stream_peer_tcp.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot_mcp {

enum class BridgeState : uint8_t {
    Disconnected,
    TcpConnecting,
    WsHandshaking,
    Connected,
};

class GameBridgeClient : public godot::Node {
    GDCLASS(GameBridgeClient, godot::Node)

public:
    GameBridgeClient();
    ~GameBridgeClient() override;

    void _ready() override;
    void _process(double delta) override;
    void _exit_tree() override;

    static void _bind_methods();

    BridgeState bridge_state() const noexcept { return state_; }

private:
    void connect_to_editor();
    void send_ws_handshake();
    void process_ws_handshake_response();
    void process_incoming_messages();
    void send_message(WsOpcode opcode, const godot::Dictionary &msg);
    void send_event(const godot::String &event_type, const godot::Dictionary &data);

    // Scene tree commands
    godot::Dictionary dispatch(const godot::String &cmd, const godot::Dictionary &params);
    godot::Dictionary handle_get_scene_tree(const godot::Dictionary &params);
    godot::Dictionary handle_get_property(const godot::Dictionary &params);
    godot::Dictionary handle_set_property(const godot::Dictionary &params);
    godot::Dictionary handle_call_method(const godot::Dictionary &params);
    godot::Dictionary handle_screenshot(const godot::Dictionary &params);
    godot::Dictionary handle_simulate_input(const godot::Dictionary &params);
    godot::Dictionary handle_set_pause(const godot::Dictionary &params);

    godot::Dictionary node_to_dict(godot::Node *node, int64_t max_depth, int64_t depth);
    godot::Node *get_scene_root();

    // Connection state
    BridgeState state_ = BridgeState::Disconnected;
    godot::Ref<godot::StreamPeerTCP> conn_;
    godot::PackedByteArray read_buf_;
    int64_t read_offset_ = 0;
    godot::String expected_accept_;

    // Config
    static constexpr uint16_t kDefaultEditorPort = 9600;
    static constexpr int kReconnectDelayMs = 2000;
    static constexpr int kMaxMessageSize = 65536;
    static constexpr int64_t kBufferLimit = 1024 * 1024;

    // Event sources
    double metrics_timer_ = 0.0;
    static constexpr double kMetricsIntervalSec = 1.0;
    godot::NodePath cached_root_path_;

    // Reconnect
    uint64_t last_disconnect_msec_ = 0;

    static int read_port();
    void reset_read_state();
};

} // namespace godot_mcp
```

### 步骤 2: game_bridge.cpp 重写

**文件**: `extensions/src/runtime/game_bridge.cpp`

完全重写。关键方法实现：

```cpp
#include "game_bridge.hpp"
#include "built_in/cmd_utils.hpp"
#include "logging.hpp"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/input_event_action.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/classes/marshalls.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/performance.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/viewport_texture.hpp>
#include <godot_cpp/classes/window.hpp>

using namespace godot;

namespace godot_mcp {

GameBridgeClient::GameBridgeClient() = default;
GameBridgeClient::~GameBridgeClient() = default;

void GameBridgeClient::_bind_methods() {}

int GameBridgeClient::read_port() {
    return read_port_from_env("GODOT_MCP_HTTP_PORT", kDefaultEditorPort);
}

void GameBridgeClient::_ready() {
    if (Engine::get_singleton()->is_editor_hint()) return;
    set_process(true);
    connect_to_editor();
}

void GameBridgeClient::_exit_tree() {
    if (conn_.is_valid()) {
        // Send close frame
        PackedByteArray close_frame = WebSocketCodec::encode(WsOpcode::Close, nullptr, 0);
        conn_->put_data(close_frame);
        conn_.unref();
    }
    state_ = BridgeState::Disconnected;
}

void GameBridgeClient::reset_read_state() {
    read_buf_.clear();
    read_offset_ = 0;
}

void GameBridgeClient::connect_to_editor() {
    conn_.instantiate();
    const int port = read_port();
    const Error err = conn_->connect_to_host("127.0.0.1", port);
    if (err != OK) {
        conn_.unref();
        state_ = BridgeState::Disconnected;
        last_disconnect_msec_ = Time::get_singleton()->get_ticks_msec();
        return;
    }
    state_ = BridgeState::TcpConnecting;
    log_info("game_bridge", String("Connecting to editor on :") + String::num_int64(port));
}

void GameBridgeClient::send_ws_handshake() {
    // Generate 16-byte random key → base64
    PackedByteArray key_bytes;
    key_bytes.resize(16);
    for (int i = 0; i < 16; ++i) {
        key_bytes.set(i, static_cast<uint8_t>(Math::randi() & 0xFF));
    }
    const String ws_key = Marshalls::get_singleton()->raw_to_base64(key_bytes);

    const String request = String("GET /bridge HTTP/1.1\r\n")
        + "Host: 127.0.0.1\r\n"
        + "Upgrade: websocket\r\n"
        + "Connection: Upgrade\r\n"
        + "Sec-WebSocket-Key: " + ws_key + "\r\n"
        + "Sec-WebSocket-Version: 13\r\n\r\n";

    conn_->put_data(request.to_utf8_buffer());
    expected_accept_ = WebSocketCodec::build_accept_value(ws_key);
    state_ = BridgeState::WsHandshaking;
}

void GameBridgeClient::process_ws_handshake_response() {
    conn_->poll();
    if (conn_->get_status() == StreamPeerTCP::STATUS_ERROR) {
        state_ = BridgeState::Disconnected;
        conn_.unref();
        return;
    }

    const int64_t avail = conn_->get_available_bytes();
    if (avail <= 0) return;

    Array chunk = conn_->get_partial_data(static_cast<int>(avail));
    if (static_cast<Error>(static_cast<int>(chunk[0])) != OK) return;

    const PackedByteArray data = chunk[1];
    const String response = String::utf8(reinterpret_cast<const char *>(data.ptr()), data.size());

    // Verify 101 Switching Protocols
    if (response.find("101") < 0) {
        log_warn("game_bridge", "WebSocket handshake failed: no 101 response");
        state_ = BridgeState::Disconnected;
        conn_.unref();
        return;
    }

    // Verify Sec-WebSocket-Accept
    if (response.find(expected_accept_) < 0) {
        log_warn("game_bridge", "WebSocket handshake failed: accept mismatch");
        state_ = BridgeState::Disconnected;
        conn_.unref();
        return;
    }

    state_ = BridgeState::Connected;
    log_info("game_bridge", "WebSocket connected to editor");

    // Send connected event
    Dictionary data_dict;
    data_dict["pid"] = OS::get_singleton()->get_process_id();
    Node *root = get_scene_root();
    if (root) data_dict["scene_root"] = String(root->get_path());
    send_event("connected", data_dict);
}

void GameBridgeClient::_process(double delta) {
    switch (state_) {
        case BridgeState::Disconnected:
            if (Time::get_singleton()->get_ticks_msec() - last_disconnect_msec_ > kReconnectDelayMs) {
                connect_to_editor();
            }
            break;

        case BridgeState::TcpConnecting:
            conn_->poll();
            if (conn_->get_status() == StreamPeerTCP::STATUS_CONNECTED) {
                send_ws_handshake();
            } else if (conn_->get_status() == StreamPeerTCP::STATUS_ERROR) {
                state_ = BridgeState::Disconnected;
                conn_.unref();
                last_disconnect_msec_ = Time::get_singleton()->get_ticks_msec();
            }
            break;

        case BridgeState::WsHandshaking:
            process_ws_handshake_response();
            break;

        case BridgeState::Connected:
            conn_->poll();
            if (conn_->get_status() != StreamPeerTCP::STATUS_CONNECTED) {
                state_ = BridgeState::Disconnected;
                conn_.unref();
                last_disconnect_msec_ = Time::get_singleton()->get_ticks_msec();
                break;
            }
            process_incoming_messages();

            // Metrics push (1Hz)
            metrics_timer_ += delta;
            if (metrics_timer_ >= kMetricsIntervalSec) {
                metrics_timer_ = 0.0;
                Dictionary metrics;
                metrics["fps"] = Performance::get_singleton()->get_monitor(Performance::TIME_FPS);
                metrics["nodes"] = Performance::get_singleton()->get_monitor(Performance::OBJECT_NODE_COUNT);
                metrics["mem_mb"] = Performance::get_singleton()->get_monitor(Performance::MEMORY_STATIC) / (1024.0 * 1024.0);
                send_event("metrics", metrics);
            }

            // Scene change detection
            Node *root = get_scene_root();
            if (root) {
                const NodePath current = root->get_path();
                if (current != cached_root_path_) {
                    cached_root_path_ = current;
                    Dictionary data;
                    data["root"] = String(current);
                    send_event("scene_changed", data);
                }
            }
            break;
    }
}

void GameBridgeClient::process_incoming_messages() {
    conn_->poll();
    const int64_t avail = conn_->get_available_bytes();
    if (avail <= 0) return;

    Array chunk = conn_->get_partial_data(static_cast<int>(avail));
    if (static_cast<Error>(static_cast<int>(chunk[0])) != OK) return;

    read_buf_.append_array(chunk[1]);
    if (read_buf_.size() > kBufferLimit) {
        log_warn("game_bridge", "Buffer overflow, resetting");
        reset_read_state();
        return;
    }

    while (read_offset_ < read_buf_.size()) {
        WsFrame frame;
        const auto consumed = WebSocketCodec::decode(
            read_buf_.ptr() + read_offset_,
            read_buf_.size() - read_offset_,
            frame);
        if (!consumed.has_value()) break;

        read_offset_ += consumed.value();

        if (frame.opcode == WsOpcode::Text) {
            const String text = String::utf8(
                reinterpret_cast<const char *>(frame.payload.ptr()),
                frame.payload.size());
            Ref<JSON> json;
            json.instantiate();
            if (json->parse(text) == OK && json->get_data().get_type() == Variant::DICTIONARY) {
                const Dictionary msg = json->get_data();
                const String type = msg.get("type", "");
                if (type == "request") {
                    const String cmd = msg.get("cmd", "");
                    const Dictionary params = msg.get("params", Dictionary());
                    const Variant id = msg.get("id", Variant());

                    Dictionary result = dispatch(cmd, params);
                    result["id"] = id;
                    result["type"] = "response";
                    send_message(WsOpcode::Text, result);
                }
            }
        } else if (frame.opcode == WsOpcode::Close) {
            state_ = BridgeState::Disconnected;
            conn_.unref();
            reset_read_state();
            return;
        } else if (frame.opcode == WsOpcode::Ping) {
            const PackedByteArray pong = WebSocketCodec::encode(
                WsOpcode::Pong, frame.payload.ptr(), frame.payload.size());
            conn_->put_data(pong);
        }
    }

    // Trim consumed bytes
    if (read_offset_ > 0) {
        const int remaining = read_buf_.size() - static_cast<int>(read_offset_);
        if (remaining > 0) {
            PackedByteArray new_buf;
            new_buf.resize(remaining);
            std::copy_n(read_buf_.ptr() + read_offset_, remaining, new_buf.ptrw());
            read_buf_ = new_buf;
        } else {
            read_buf_.clear();
        }
        read_offset_ = 0;
    }
}

void GameBridgeClient::send_message(WsOpcode opcode, const Dictionary &msg) {
    if (state_ != BridgeState::Connected || !conn_.is_valid()) return;
    const String json = JSON::stringify(msg);
    const PackedByteArray payload = json.to_utf8_buffer();
    const PackedByteArray frame = WebSocketCodec::encode(opcode, payload.ptr(), payload.size());
    conn_->put_data(frame);
}

void GameBridgeClient::send_event(const String &event_type, const Dictionary &data) {
    Dictionary msg;
    msg["type"] = "event";
    msg["event"] = event_type;
    msg["data"] = data;
    send_message(WsOpcode::Text, msg);
}

// ── Command dispatch (existing game_bridge logic, unchanged) ──

Node *GameBridgeClient::get_scene_root() {
    SceneTree *st = get_tree();
    return st ? st->get_current_scene() : nullptr;
}

// ... dispatch(), handle_get_scene_tree(), handle_get_property(), etc.
// ... (移植现有 game_bridge.cpp 中的 dispatch 逻辑，不需要修改)

} // namespace godot_mcp
```

### 步骤 3: register_types.cpp 更新

**文件**: `extensions/src/register_types.cpp`

将 `GameBridgeNode` 的注册和自动创建改为 `GameBridgeClient`：

```cpp
// 第 26 行附近
GDREGISTER_CLASS(godot_mcp::GameBridgeClient);

if (!godot::Engine::get_singleton()->is_editor_hint()) {
    godot_mcp::GameBridgeClient *bridge = memnew(godot_mcp::GameBridgeClient);
    bridge->set_name("GameBridgeClient");
    bridge->call_deferred("_self_add");
}
```

同时更新 include：

```cpp
#include "runtime/game_bridge.hpp"  // 文件名不变，类名变了
```

---

## K: 多实例路由 + Dock UI + 消除 bridge_port

### 步骤 1: editor_plugin.hpp 清理

**文件**: `extensions/src/editor_plugin.hpp`

删除：

```cpp
#include "runtime/bridge.hpp"  // 删除 — bridge.hpp 将被删除
// RuntimeBridge runtime_bridge_;  // 删除
// int bridge_port_ = 9601;        // 删除
```

新增：

```cpp
#include "server/ipc/http_server.hpp"  // 已有，确认包含 WsConnection
```

### 步骤 2: editor_plugin.cpp 清理

**文件**: `extensions/src/editor_plugin.cpp`

删除：
- `load_config()` 中 `bridge_port` 相关的读取和写入
- `save_config()` 中 `bridge_port` 相关的写入
- `_try_bridge_connect()` 整个方法
- `_process()` 中 `runtime_bridge_.poll()` 调用
- `_exit_tree()` 中 `runtime_bridge_.disconnect()` 调用
- `restart_server()` 中 `runtime_bridge_` 相关代码

新增（`_enter_tree()` 中，WebSocket 事件回调）：

```cpp
    http_server_.set_bridge_event_callback(
        [this](int instance_id, const String &event, const Dictionary &data) {
            // MCP notification → SSE
            const String method = "notifications/game/" + event;
            Dictionary params;
            params["instance_id"] = instance_id;
            params["data"] = data;
            mcp_handler_.enqueue_event(McpHandler::make_notification(method, params));

            // UI update
            if (event == "connected" || event == "disconnected") {
                mcp_dock_->update_instance_list(http_server_.get_ws_instances());
            }
            if (event == "metrics") {
                mcp_dock_->update_instance_metrics(instance_id, data);
            }
        });
```

### 步骤 3: GetGameInstancesTool

**新文件**: `extensions/src/built_in/tools/meta/get_game_instances.hpp`

```cpp
#pragma once

#include "built_in/tool_base.hpp"

namespace godot_mcp {

class GetGameInstancesTool : public ITool {
public:
    godot::String name() const noexcept override { return "get_game_instances"; }
    godot::String brief() const noexcept override { return "List connected game instances"; }
    godot::String description() const noexcept override {
        return "Returns all game instances currently connected via WebSocket bridge. "
               "Each instance has an id, pid, scene_root, and connected status.";
    }
    godot::String category() const noexcept override { return "meta_tools"; }
    bool is_meta() const noexcept override { return true; }

protected:
    godot::Dictionary execute_impl(const ToolContext &ctx) override;
};

} // namespace godot_mcp
```

**新文件**: `extensions/src/built_in/tools/meta/get_game_instances.cpp`

```cpp
#include "get_game_instances.hpp"
#include "built_in/cmd_utils.hpp"
#include "server/registry/handler_registry.hpp"
#include "server/ipc/http_server.hpp"

using namespace godot;

namespace godot_mcp {

Dictionary GetGameInstancesTool::execute_impl(const ToolContext &ctx) {
    // Access HttpServer through HandlerRegistry
    // Need to add a getter: registry_->get_http_server()
    // Or store a pointer in the tool

    // For now, use the HandlerRegistry to get instance info
    Array instances;
    // ... iterate ws_connections_ and build instance list ...

    Dictionary data;
    data["instances"] = instances;
    data["count"] = instances.size();
    return ToolResult::ok(data);
}

} // namespace godot_mcp
```

### 步骤 4: 注册新工具

**文件**: `extensions/src/built_in/tools/register/register_meta.hpp`

添加：

```cpp
GODOT_MCP_TOOL(GetGameInstancesTool, false)
```

**文件**: `extensions/src/built_in/register_itools.cpp`

添加：

```cpp
#include "built_in/tools/meta/get_game_instances.hpp"
```

### 步骤 5: 删除 bridge.hpp/cpp

**文件**: `extensions/src/runtime/bridge.hpp` — 删除
**文件**: `extensions/src/runtime/bridge.cpp` — 删除

从 CMakeLists.txt 中移除这两个文件的引用（如果有的话）。

### 步骤 6: Dock UI 实例列表

**文件**: `extensions/src/ui/mcp_dock.hpp`

新增：

```cpp
godot::Label *instance_list_label_ = nullptr;
void update_instance_list(const Array &instances);
void update_instance_metrics(int instance_id, const Dictionary &metrics);
```

**文件**: `extensions/src/ui/mcp_dock.cpp`

在 Bridge 状态区域下方添加实例列表显示。在构造函数中创建 `instance_list_label_`，在 `update_status()` 中更新。

---

## 验证清单

### Sprint 3 完整验证

1. **编译通过**：`uv run python main.py build`
2. **WebSocket 握手**：运行游戏 → 编辑器日志显示 `WebSocket instance #1 connected`
3. **命令响应**：通过 MCP 调用 `get_game_scene_tree(instance_id=1)` → 返回场景树
4. **多实例**：运行 2 个游戏实例 → `get_game_instances` 返回 2 条记录
5. **Metrics 推送**：MCP Console 显示 FPS/节点数/内存
6. **Scene change**：切换游戏场景 → 收到 `scene_changed` 事件
7. **断连恢复**：停止游戏 → 编辑器日志显示 `disconnected` → 重新运行 → 自动重连
8. **bridge_port 消除**：ProjectSettings 中不再有 `godot_mcp/bridge_port`
9. **Dock 显示**：右侧面板显示连接的实例列表
