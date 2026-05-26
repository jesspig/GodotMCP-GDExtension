// =====================================================================
// ipc/ws_server.hpp — Synchronous WebSocket server polled per-frame.
//
// Uses Godot 4.6's built-in TCPServer + WebSocketPeer to accept inbound
// connections from the Python MCP server and route JSON requests through
// the HandlerRegistry. Everything runs on the editor's main thread — the
// Rust implementation needed a tokio runtime + main-thread dispatcher
// because of Rust-specific `bind_mut` restrictions, but those don't
// apply to godot-cpp, so this version is much simpler.
//
// Protocol:
//   1. Listen on 127.0.0.1:<port>.
//   2. On each accept, send a {"event": "godot_ready", "data": {...}}
//      notification carrying the engine + plugin versions.
//   3. For each incoming text packet, parse as JSON, dispatch, and reply
//      with a JSON response (see protocol/ipc_types.hpp for the schema).
// =====================================================================

#pragma once

#include <godot_cpp/classes/tcp_server.hpp>
#include <godot_cpp/classes/web_socket_peer.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/hash_set.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace godot_mcp {

class HandlerRegistry;

class WsServer {
public:
    WsServer();
    ~WsServer();

    // Bind the listening socket. Returns true on success.
    bool start(int port, HandlerRegistry *registry);

    // Stop the server, close all peers.
    void stop();

    // Per-frame work: accept new peers, poll existing peers, dispatch.
    // Must be called on the main thread.
    void poll();

    // Broadcast a notification dict to every connected peer.
    void broadcast(const godot::Dictionary &notification);

    bool is_listening() const;

private:
    // Send a JSON object to a single peer.
    static void send_dict(godot::Ref<godot::WebSocketPeer> peer,
                          const godot::Dictionary &payload);

    // Handle one inbound packet -> reply payload.
    void handle_packet(int peer_id,
                       godot::Ref<godot::WebSocketPeer> peer,
                       const godot::String &text);

    // Build a {"event":"godot_ready"} notification.
    godot::Dictionary make_ready_notification() const;

    godot::Ref<godot::TCPServer> tcp_server_;
    godot::HashMap<int, godot::Ref<godot::WebSocketPeer>> peers_;
    godot::HashSet<int> greeted_;
    int next_peer_id_ = 0;
    HandlerRegistry *registry_ = nullptr;
    int port_ = 0;
};

}  // namespace godot_mcp
