#pragma once

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/ref.hpp>
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
    void connect_to_editor();
    void disconnect_from_editor();
    void read_from_editor();
    void reset_read_state_internal();
    void send_response(const godot::Dictionary &msg);
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

    godot::Ref<godot::StreamPeerTCP> connection_;
    godot::PackedByteArray read_buf_;
    int64_t read_offset_ = 0;
    int port_ = 9601;
    uint64_t connect_retry_msec_ = 0;
    static constexpr int64_t BUFFER_LIMIT = 1024 * 1024;
    static constexpr int64_t MAX_MESSAGE_SIZE = 65536;
};

} // namespace godot_mcp
