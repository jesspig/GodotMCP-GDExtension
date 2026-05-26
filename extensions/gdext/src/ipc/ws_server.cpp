// =====================================================================
// ipc/ws_server.cpp — WebSocket server implementation.
//
// Per-frame flow (poll()):
//   1. Accept any pending TCP connections, hand each socket to a new
//      WebSocketPeer in server mode, send a "godot_ready" notification.
//   2. For every existing peer:
//        - peer->poll() to advance the state machine.
//        - Drain inbound text packets and dispatch each as a tool call.
//        - Drop peers whose state has reached STATE_CLOSED.
//
// The dispatch protocol is intentionally small — there is exactly one
// method ("tool_call") plus opaque notifications. This matches the
// Python server's expectations one-for-one.
// =====================================================================

#include "ws_server.hpp"

#include "../commands/cmd_utils.hpp"
#include "../commands/handler_registry.hpp"
#include "../logging.hpp"
#include "../protocol/ipc_types.hpp"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/classes/stream_peer.hpp>
#include <godot_cpp/classes/stream_peer_tcp.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

namespace godot_mcp {

WsServer::WsServer() = default;

WsServer::~WsServer() {
    stop();
}

bool WsServer::start(int port, HandlerRegistry *registry) {
    stop();
    registry_ = registry;
    port_ = port;

    tcp_server_.instantiate();
    const Error err = tcp_server_->listen((uint16_t)port, "127.0.0.1");
    if (err != OK) {
        log_error("ws", String("Failed to listen on 127.0.0.1:") + String::num_int64(port) +
                          String(" (err=") + String::num_int64((int64_t)err) + String(")"));
        tcp_server_.unref();
        return false;
    }

    log_info("ws", String("Listening on 127.0.0.1:") + String::num_int64(port));
    return true;
}

void WsServer::stop() {
    // Close every peer cleanly so the Python side sees a proper FIN.
    for (KeyValue<int, Ref<WebSocketPeer>> &kv : peers_) {
        Ref<WebSocketPeer> peer = kv.value;
        if (peer.is_valid()) {
            peer->close(1000, "shutdown");
        }
    }
    peers_.clear();

    if (tcp_server_.is_valid()) {
        tcp_server_->stop();
        tcp_server_.unref();
    }
    registry_ = nullptr;
}

bool WsServer::is_listening() const {
    return tcp_server_.is_valid() && tcp_server_->is_listening();
}

void WsServer::poll() {
    if (!tcp_server_.is_valid() || !tcp_server_->is_listening()) {
        return;
    }

    // --- Accept any pending TCP connections -----------------------------
    while (tcp_server_->is_connection_available()) {
        Ref<StreamPeerTCP> tcp = tcp_server_->take_connection();
        if (tcp.is_null()) {
            break;
        }
        Ref<WebSocketPeer> peer;
        peer.instantiate();
        const Error err = peer->accept_stream(tcp);
        if (err != OK) {
            log_warn("ws", String("accept_stream failed (err=") +
                                   String::num_int64((int64_t)err) + String(")"));
            continue;
        }
        const int peer_id = next_peer_id_++;
        peers_[peer_id] = peer;
        log_info("ws", String("Peer #") + String::num_int64(peer_id) + String(" connected"));

        // Tell the peer who we are. Send is deferred until the WS handshake
        // completes, so push it now — WebSocketPeer queues the frame and
        // emits it once STATE_OPEN is reached.
        send_dict(peer, make_ready_notification());
    }

    // --- Poll existing peers --------------------------------------------
    // Collect dead peers first so we don't mutate the map while iterating.
    Vector<int> dead;
    for (KeyValue<int, Ref<WebSocketPeer>> &kv : peers_) {
        const int peer_id = kv.key;
        Ref<WebSocketPeer> peer = kv.value;
        if (peer.is_null()) {
            dead.push_back(peer_id);
            continue;
        }
        peer->poll();
        const WebSocketPeer::State state = peer->get_ready_state();
        if (state == WebSocketPeer::STATE_CLOSED) {
            log_info("ws", String("Peer #") + String::num_int64(peer_id) + String(" disconnected"));
            dead.push_back(peer_id);
            continue;
        }
        if (state != WebSocketPeer::STATE_OPEN) {
            continue;  // Still handshaking.
        }
        while (peer->get_available_packet_count() > 0) {
            const PackedByteArray pkt = peer->get_packet();
            // We only accept text frames carrying JSON.
            if (peer->was_string_packet()) {
                const String text = pkt.get_string_from_utf8();
                handle_packet(peer_id, peer, text);
            } else {
                log_warn("ws", String("Ignoring binary packet from peer #") +
                                       String::num_int64(peer_id));
            }
        }
    }
    for (int i = 0; i < dead.size(); ++i) {
        peers_.erase(dead[i]);
    }
}

void WsServer::broadcast(const Dictionary &notification) {
    for (KeyValue<int, Ref<WebSocketPeer>> &kv : peers_) {
        Ref<WebSocketPeer> peer = kv.value;
        if (peer.is_valid() && peer->get_ready_state() == WebSocketPeer::STATE_OPEN) {
            send_dict(peer, notification);
        }
    }
}

// ---------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------

void WsServer::send_dict(Ref<WebSocketPeer> peer, const Dictionary &payload) {
    if (peer.is_null()) {
        return;
    }
    const String text = JSON::stringify(payload);
    // send_text is the convenience overload — we want a text frame, not
    // a binary one, so the Python side can JSON-decode it directly.
    peer->send_text(text);
}

void WsServer::handle_packet(int peer_id,
                             Ref<WebSocketPeer> peer,
                             const String &text) {
    // --- Parse JSON ------------------------------------------------------
    Ref<JSON> json;
    json.instantiate();
    const Error parse_err = json->parse(text);
    if (parse_err != OK) {
        log_warn("ws", String("Peer #") + String::num_int64(peer_id) +
                               String(" sent invalid JSON: ") + json->get_error_message());
        send_dict(peer,
                  make_response(Variant(), make_error_result(kErrCodeInvalidRequest,
                                                              json->get_error_message())));
        return;
    }
    const Variant root_v = json->get_data();
    if (root_v.get_type() != Variant::DICTIONARY) {
        send_dict(peer, make_response(Variant(),
                                       make_error_result(kErrCodeInvalidRequest,
                                                          "Request must be a JSON object")));
        return;
    }
    const Dictionary req = root_v;
    const Variant req_id = req.has("id") ? req["id"] : Variant();

    // --- Validate envelope ----------------------------------------------
    const String method = req.has("method") ? String(req["method"]) : String();
    if (method != "tool_call") {
        send_dict(peer, make_response(req_id,
                                       make_error_result(kErrCodeInvalidRequest,
                                                          String("Unknown method: ") + method)));
        return;
    }
    if (!req.has("params") || req["params"].get_type() != Variant::DICTIONARY) {
        send_dict(peer, make_response(req_id,
                                       make_error_result(kErrCodeInvalidRequest,
                                                          "Missing or invalid 'params'")));
        return;
    }
    const Dictionary params = req["params"];
    const String tool = params.has("tool") ? String(params["tool"]) : String();
    const Dictionary args = params.has("args") && params["args"].get_type() == Variant::DICTIONARY
                                    ? Dictionary(params["args"])
                                    : Dictionary();

    if (tool.is_empty()) {
        send_dict(peer, make_response(req_id,
                                       make_error_result(kErrCodeInvalidRequest,
                                                          "Missing 'tool' name")));
        return;
    }

    // --- Look up handler -------------------------------------------------
    if (!registry_) {
        send_dict(peer, make_response(req_id,
                                       make_error_result(kErrCodeInternal,
                                                          "Registry is not initialised")));
        return;
    }
    const CommandFn *fn = registry_->find(tool);
    if (!fn) {
        send_dict(peer, make_response(req_id,
                                       make_error_result(kErrCodeUnknownTool,
                                                          String("Unknown tool: ") + tool)));
        return;
    }

    // --- Execute handler -------------------------------------------------
    // Handlers report failure by returning a Dictionary that contains an
    // "error" string, mirroring the Rust convention. Any C++ exception
    // (rare in godot-cpp code but possible) maps to a TOOL_FAILED error.
    Dictionary data;
    try {
        data = (*fn)(args);
    } catch (const std::exception &e) {
        send_dict(peer, make_response(req_id,
                                       make_error_result(kErrCodeToolFailed, String(e.what()))));
        return;
    }

    if (data.has("error")) {
        const String msg = data["error"];
        send_dict(peer, make_response(req_id, make_error_result(kErrCodeToolFailed, msg)));
        return;
    }

    send_dict(peer, make_response(req_id, make_success_result(data)));
}

Dictionary WsServer::make_ready_notification() const {
    Dictionary data;
    data["port"] = port_;
    if (registry_) {
        data["engine_version"] = registry_->engine_version();
        data["plugin_version"] = registry_->plugin_version();
    }
    return make_notification("godot_ready", data);
}

}  // namespace godot_mcp
