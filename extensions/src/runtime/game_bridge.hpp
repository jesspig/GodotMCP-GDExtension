#pragma once

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/classes/tcp_server.hpp>
#include <godot_cpp/classes/stream_peer_tcp.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/array.hpp>

namespace godot_mcp {

class GameBridgeNode : public godot::Node {
    GDCLASS(GameBridgeNode, godot::Node)

public:
    GameBridgeNode();
    ~GameBridgeNode() override;

    void _ready() override;
    void _process(double delta) override;
    void _exit_tree() override;

    void _self_add();

    static void _bind_methods();

private:
    void start_server();
    void stop_server();
    void accept_clients();
    void read_clients();
    void reset_read_state_internal();
    void send_response(const godot::Ref<godot::StreamPeerTCP> &client, const godot::Dictionary &msg);
    godot::Dictionary dispatch(const godot::String &cmd, const godot::Dictionary &params);

    godot::Dictionary handle_get_scene_tree(const godot::Dictionary &params);
    godot::Dictionary handle_get_property(const godot::Dictionary &params);
    godot::Dictionary handle_set_property(const godot::Dictionary &params);
    godot::Dictionary handle_call_method(const godot::Dictionary &params);
    godot::Dictionary handle_screenshot(const godot::Dictionary &params);
    godot::Dictionary handle_simulate_input(const godot::Dictionary &params);
    godot::Dictionary handle_set_pause(const godot::Dictionary &params);

    godot::Dictionary node_to_dict(godot::Node *node, int64_t max_depth, int64_t depth);
    static int read_port();
    godot::Node *get_scene_root();

    godot::Ref<godot::TCPServer> server_;
    godot::Ref<godot::StreamPeerTCP> client_;
    godot::PackedByteArray read_buf_;
    int64_t read_offset_ = 0;
    int port_ = 9601;
    static constexpr int64_t BUFFER_LIMIT = 1024 * 1024;
};

} // namespace godot_mcp
