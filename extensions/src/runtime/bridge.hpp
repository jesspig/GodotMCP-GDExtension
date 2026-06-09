#pragma once

#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/classes/stream_peer_tcp.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot_mcp {

class RuntimeBridge {
public:
    enum Status { DISCONNECTED, CONNECTING, CONNECTED };

    RuntimeBridge();
    ~RuntimeBridge();

    void poll();
    void connect();
    void disconnect();
    bool is_connected() const { return status_ == CONNECTED; }
    Status status() const { return status_; }

    godot::Dictionary send_command(const godot::String &cmd, const godot::Dictionary &params, int timeout_ms = 5000);

    void set_port(int port) { port_ = port; }
    int port() const { return port_; }

private:
    godot::Dictionary read_response(int timeout_ms);

    Status status_ = DISCONNECTED;
    godot::Ref<godot::StreamPeerTCP> tcp_;
    int port_ = 9601;
    int next_id_ = 1;
};

} // namespace godot_mcp
