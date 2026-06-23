#pragma once

#include <functional>
#include <optional>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/classes/stream_peer_tcp.hpp>
#include <godot_cpp/classes/tcp_server.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>

namespace godot_mcp {

class McpHandler;

constexpr int64_t BRIDGE_MAX_MESSAGE_SIZE = 4 * 1024 * 1024; // 4MB — screenshots need more than 64KB
constexpr int64_t BRIDGE_BUFFER_LIMIT = 1024 * 1024;

class RuntimeBridgeServer {
public:
    enum Status { DISCONNECTED, ACCEPTING, CONNECTED };

    RuntimeBridgeServer();
    ~RuntimeBridgeServer();

    void poll();
    void start(int port);
    void stop();
    bool is_connected() const { return status_ == CONNECTED && !instances_.is_empty(); }
    bool has_instances() const { return !instances_.is_empty(); }
    bool has_instance(int instance_id) const { return instances_.has(instance_id); }
    Status status() const { return status_; }

    godot::Dictionary send_command_async(int instance_id,
                                          const godot::String &cmd,
                                          const godot::Dictionary &params);

    // Synchronous variant: sends command and polls briefly for response.
    // For fast commands that complete within a few frames (scene tree, properties, input).
    // Returns empty Dictionary if response not ready within budget.
    godot::Dictionary send_command_sync(int instance_id,
                                         const godot::String &cmd,
                                         const godot::Dictionary &params,
                                         int max_poll_ms = 200);

    godot::Array get_connected_instances() const;
    int instance_count() const { return static_cast<int>(instances_.size()); }
    int get_default_instance_id() const;

    using ResponseCallback = std::function<void(const godot::Dictionary&)>;
    void set_response_callback(ResponseCallback cb) { response_cb_ = std::move(cb); }

    void set_port(int port) { port_ = port; }
    int port() const { return port_; }

    void set_mcp_handler(McpHandler *handler) { mcp_handler_ = handler; }

    static godot::Dictionary make_response(const godot::Dictionary &raw);

    // Bridge watcher for WaitForBridgeTool (frame-driven, non-blocking)
    void start_watcher(const godot::Variant &jsonrpc_id, int timeout_ms);

private:
    struct GameInstance {
        int id;
        godot::Ref<godot::StreamPeerTCP> connection;
        uint64_t connected_at;

        enum ReadState { READ_HEADER, READ_BODY };
        ReadState read_state_ = READ_HEADER;
        int64_t msg_length_ = 0;
        int64_t body_received_ = 0;
        godot::PackedByteArray read_buf_;
        int64_t read_offset_ = 0;
    };

    struct PendingRequest {
        int64_t id;
        int instance_id;
        uint64_t deadline_msec;
        bool completed = false;
        bool sync = false;                 // true = synchronous caller waiting, skip callback
        godot::Dictionary response;
    };

    struct BridgeWatcher {
        bool active = false;
        godot::Variant jsonrpc_id;
        uint64_t deadline_msec = 0;
        McpHandler *handler = nullptr;
    };

    void accept_new_connections();
    int add_instance(const godot::Ref<godot::StreamPeerTCP> &conn);
    void remove_instance(int instance_id);

    void send_only(int instance_id,
                   const godot::String &cmd,
                   const godot::Dictionary &params,
                   int64_t id);
    void accumulate_read_data(int instance_id, GameInstance &inst);
    void process_complete_messages(int instance_id, GameInstance &inst);
    void process_timeouts(uint64_t now);
    void error_all_pending(const godot::String &code, const godot::String &msg);
    void error_instance_pending(int instance_id,
                                const godot::String &code,
                                const godot::String &msg);
    void reset_read_state(GameInstance &inst);
    void check_watcher();

    Status status_ = DISCONNECTED;
    godot::Ref<godot::TCPServer> server_;
    godot::HashMap<int, GameInstance> instances_;
    godot::HashMap<int64_t, PendingRequest> pending_;
    int next_instance_id_ = 1;
    int port_ = 9601;
    int64_t next_request_id_ = 1;
    uint64_t listening_since_ = 0;
    ResponseCallback response_cb_;
    McpHandler *mcp_handler_ = nullptr;
    BridgeWatcher watcher_;
};

} // namespace godot_mcp
