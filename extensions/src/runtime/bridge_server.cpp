#include "bridge_server.hpp"
#include "built_in/cmd_utils/error_codes.hpp"
#include "logging.hpp"
#include "server/mcp/mcp_handler.hpp"

#include <algorithm>
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>

using namespace godot;

namespace godot_mcp {

RuntimeBridgeServer::RuntimeBridgeServer() = default;
RuntimeBridgeServer::~RuntimeBridgeServer() = default;

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void RuntimeBridgeServer::start(int port) {
    if (status_ != DISCONNECTED) stop();

    port_ = port;
    server_.instantiate();
    Error err = server_->listen(port_, "127.0.0.1");
    if (err != OK) {
        log_error("bridge", String("RuntimeBridgeServer failed to listen on :") + String::num_int64(port_));
        server_.unref();
        status_ = DISCONNECTED;
        return;
    }
    status_ = ACCEPTING;
    listening_since_ = Time::get_singleton()->get_ticks_msec();
    log_info("bridge", String("RuntimeBridgeServer listening on :") + String::num_int64(port_));
}

void RuntimeBridgeServer::stop() {
    // Error all pending requests
    if (!pending_.is_empty()) {
        error_all_pending("SERVER_STOPPED", "Bridge server stopped");
    }

    // Disconnect all instances
    for (auto &kv : instances_) {
        GameInstance &inst = kv.value;
        if (inst.connection.is_valid()) {
            inst.connection->disconnect_from_host();
            inst.connection.unref();
        }
    }
    instances_.clear();

    // Stop TCP server
    if (server_.is_valid()) {
        server_->stop();
        server_.unref();
    }

    watcher_.active = false;

    status_ = DISCONNECTED;
    log_info("bridge", "RuntimeBridgeServer stopped");
}

// ---------------------------------------------------------------------------
// poll() — called each frame from McpEditorPlugin::_process()
// ---------------------------------------------------------------------------

void RuntimeBridgeServer::poll() {
    if (status_ == DISCONNECTED) return;

    // Step 1: Accept new connections
    accept_new_connections();

    // Step 2: Update status if connections exist
    if (status_ == ACCEPTING && !instances_.is_empty()) {
        status_ = CONNECTED;
    }

    // Step 3: Poll each instance
    Vector<int> dead;
    for (auto &kv : instances_) {
        const int id = kv.key;
        GameInstance &inst = kv.value;

        if (!inst.connection.is_valid()) {
            dead.push_back(id);
            continue;
        }

        inst.connection->poll();
        StreamPeerTCP::Status s = inst.connection->get_status();
        if (s == StreamPeerTCP::STATUS_ERROR || s == StreamPeerTCP::STATUS_NONE) {
            error_instance_pending(id, "CONNECTION_LOST", "Instance disconnected");
            dead.push_back(id);
            continue;
        }

        accumulate_read_data(id, inst);
        process_complete_messages(id, inst);
    }

    for (int id : dead) {
        remove_instance(id);
    }

    // Step 4: Handle state transition when last instance leaves
    if (status_ == CONNECTED && instances_.is_empty()) {
        status_ = ACCEPTING;
    }

    // Step 5: Timeout cleanup
    const uint64_t now = Time::get_singleton()->get_ticks_msec();
    process_timeouts(now);

    // Step 6: Check bridge watcher
    check_watcher();
}

// ---------------------------------------------------------------------------
// Connection management
// ---------------------------------------------------------------------------

void RuntimeBridgeServer::accept_new_connections() {
    if (server_.is_null()) return;

    while (server_->is_connection_available()) {
        Ref<StreamPeerTCP> conn = server_->take_connection();
        if (conn.is_null()) break;

        conn->set_no_delay(true);
        const int id = add_instance(conn);
        log_info("bridge", String("Game instance ") + String::num_int64(id) +
                               String(" connected (total: ") + String::num_int64(instances_.size()) +
                               String(")"));
    }
}

int RuntimeBridgeServer::add_instance(const Ref<StreamPeerTCP> &conn) {
    const int id = next_instance_id_++;
    GameInstance inst;
    inst.id = id;
    inst.connection = conn;
    inst.connected_at = Time::get_singleton()->get_ticks_msec();
    instances_[id] = inst;

    // Remove stale duplicate connections from the same peer
    // (same IP:port within 5s to avoid race with rapid reconnect).
    const uint64_t now = inst.connected_at;
    const String peer = conn->get_connected_host() + ":" + String::num_int64(conn->get_connected_port());
    Vector<int> stale;
    for (const auto &kv : instances_) {
        if (kv.key == id) continue;
        if (!kv.value.connection.is_valid()) continue;
        String other_peer = kv.value.connection->get_connected_host() + ":" +
                            String::num_int64(kv.value.connection->get_connected_port());
        if (other_peer == peer && now - kv.value.connected_at < 5000) {
            stale.push_back(kv.key);
        }
    }
    for (int sid : stale) {
        remove_instance(sid);
    }

    return id;
}

void RuntimeBridgeServer::remove_instance(int instance_id) {
    auto it = instances_.find(instance_id);
    if (it == instances_.end()) return;

    if (it->value.connection.is_valid()) {
        it->value.connection->disconnect_from_host();
        it->value.connection.unref();
    }
    instances_.erase(instance_id);
    log_info("bridge", String("Game instance ") + String::num_int64(instance_id) +
                           String(" disconnected (remaining: ") + String::num_int64(instances_.size()) +
                           String(")"));
}

// ---------------------------------------------------------------------------
// Async command interface
// ---------------------------------------------------------------------------

Dictionary RuntimeBridgeServer::send_command_async(int instance_id,
                                                     const String &cmd,
                                                     const Dictionary &params) {
    if (!instances_.has(instance_id)) {
        Dictionary r;
        r["pending"] = false;
        r["ok"] = false;
        r["error"] = String("Game instance ") + String::num_int64(instance_id) + String(" not found");
        return make_response(r);
    }

    const int64_t id = next_request_id_++;
    send_only(instance_id, cmd, params, id);

    PendingRequest pr;
    pr.id = id;
    pr.instance_id = instance_id;
    pr.deadline_msec = Time::get_singleton()->get_ticks_msec() + DEFAULT_TIMEOUT_MS;
    pending_[id] = pr;

    Dictionary result;
    result["pending"] = id;
    return result;
}

void RuntimeBridgeServer::send_only(int instance_id,
                                     const String &cmd,
                                     const Dictionary &params,
                                     int64_t id) {
    auto it = instances_.find(instance_id);
    if (it == instances_.end()) return;

    Dictionary msg;
    msg["cmd"] = cmd;
    msg["params"] = params;
    msg["id"] = id;

    String json_str = JSON::stringify(msg);
    PackedByteArray json_bytes = json_str.to_utf8_buffer();

    const uint32_t len = static_cast<uint32_t>(json_bytes.size());
    PackedByteArray frame;
    frame.resize(4 + json_bytes.size());
    auto *p = frame.ptrw();
    p[0] = static_cast<uint8_t>((len >> 24) & 0xFF);
    p[1] = static_cast<uint8_t>((len >> 16) & 0xFF);
    p[2] = static_cast<uint8_t>((len >> 8) & 0xFF);
    p[3] = static_cast<uint8_t>(len & 0xFF);
    std::copy_n(json_bytes.ptr(), json_bytes.size(), p + 4);

    Error err = it->value.connection->put_data(frame);
    if (err != OK) {
        error_instance_pending(instance_id, "SEND_FAILED", "Failed to send command to instance");
        remove_instance(instance_id);
    }
}

// ---------------------------------------------------------------------------
// Per-instance async read (non-blocking, frame-driven)
// ---------------------------------------------------------------------------

void RuntimeBridgeServer::accumulate_read_data(int instance_id, GameInstance &inst) {
    const int64_t avail = inst.connection->get_available_bytes();
    if (avail <= 0) return;

    Array chunk = inst.connection->get_partial_data(static_cast<int>(avail));
    if (static_cast<Error>(static_cast<int>(chunk[0])) != OK) return;

    PackedByteArray data = chunk[1];
    inst.read_buf_.append_array(data);

    if (inst.read_buf_.size() > BRIDGE_BUFFER_LIMIT) {
        log_warn("bridge", String("Instance ") + String::num_int64(instance_id) +
                               String(" buffer overflow, resetting"));
        reset_read_state(inst);
    }
}

void RuntimeBridgeServer::process_complete_messages(int instance_id, GameInstance &inst) {
    while (inst.read_offset_ + 4 <= static_cast<int64_t>(inst.read_buf_.size())) {
        if (inst.read_state_ == GameInstance::READ_HEADER) {
            const uint8_t *p = inst.read_buf_.ptr() + inst.read_offset_;
            inst.msg_length_ = (static_cast<uint32_t>(p[0]) << 24) |
                               (static_cast<uint32_t>(p[1]) << 16) |
                               (static_cast<uint32_t>(p[2]) << 8) |
                                static_cast<uint32_t>(p[3]);

            if (inst.msg_length_ <= 0 || inst.msg_length_ > BRIDGE_MAX_MESSAGE_SIZE) {
                log_warn("bridge", String("Instance ") + String::num_int64(instance_id) +
                                       String(": invalid message length ") +
                                       String::num_int64(inst.msg_length_));
                reset_read_state(inst);
                return;
            }
            inst.read_state_ = GameInstance::READ_BODY;
            inst.read_offset_ += 4;
            inst.body_received_ = 0;
        }

        const int64_t remaining = inst.read_buf_.size() - inst.read_offset_;
        if (remaining < inst.msg_length_) break;

        // Complete message body
        const int64_t body_start = inst.read_offset_;
        inst.read_offset_ += inst.msg_length_;
        inst.read_state_ = GameInstance::READ_HEADER;

        String text;
        text.parse_utf8(reinterpret_cast<const char *>(inst.read_buf_.ptr() + body_start),
                        static_cast<int>(inst.msg_length_));

        Ref<JSON> json;
        json.instantiate();
        if (json->parse(text) != OK) {
            log_warn("bridge", String("Instance ") + String::num_int64(instance_id) +
                                   String(": invalid JSON in response"));
            continue;
        }

        Variant parsed = json->get_data();
        if (parsed.get_type() != Variant::DICTIONARY) {
            log_warn("bridge", "Response is not a dictionary");
            continue;
        }

        Dictionary resp = parsed;
        const int64_t response_id = static_cast<int64_t>(resp.get("id", (int64_t)0));

        auto pit = pending_.find(response_id);
        if (pit != pending_.end()) {
            // Store response in pending request so sync readers can access it
            pit->value.completed = true;
            pit->value.response = resp;

            if (pit->value.sync) {
                // Sync request — let send_command_sync() read and erase
            } else {
                // Async request — notify via callback and clean up
                if (response_cb_) {
                    Dictionary event;
                    event["id"] = response_id;
                    event["instance_id"] = instance_id;
                    if (resp.has("error")) {
                        event["error"] = resp["error"];
                    } else {
                        event["result"] = resp;
                    }
                    response_cb_(event);
                }
                pending_.erase(response_id);
            }
        } else {
            log_warn("bridge", String("Instance ") + String::num_int64(instance_id) +
                                   String(": response ID not in pending map"));
        }
    }

    // Trim processed bytes
    if (inst.read_offset_ > 0) {
        if (inst.read_offset_ >= static_cast<int64_t>(inst.read_buf_.size())) {
            inst.read_buf_.clear();
        } else {
            const int64_t remaining = inst.read_buf_.size() - inst.read_offset_;
            PackedByteArray new_buf;
            new_buf.resize(static_cast<int>(remaining));
            std::copy_n(inst.read_buf_.ptr() + inst.read_offset_, remaining, new_buf.ptrw());
            inst.read_buf_ = new_buf;
        }
        inst.read_offset_ = 0;
    }
}

void RuntimeBridgeServer::reset_read_state(GameInstance &inst) {
    inst.read_buf_.clear();
    inst.read_offset_ = 0;
    inst.read_state_ = GameInstance::READ_HEADER;
    inst.msg_length_ = 0;
    inst.body_received_ = 0;
}

// ---------------------------------------------------------------------------
// Timeout processing
// ---------------------------------------------------------------------------

void RuntimeBridgeServer::process_timeouts(uint64_t now) {
    Vector<int64_t> timed_out;
    for (const auto &kv : pending_) {
        // Skip sync requests — their timeout is handled by send_command_sync()
        if (kv.value.sync) continue;
        if (kv.value.deadline_msec > 0 && now > kv.value.deadline_msec) {
            timed_out.push_back(kv.key);
        }
    }

    for (int64_t id : timed_out) {
        if (response_cb_) {
            Dictionary error_response;
            error_response["id"] = id;
            error_response["instance_id"] = pending_[id].instance_id;
            Dictionary error;
            error["code"] = String(error_codes::TIMEOUT);
            error["message"] = "Bridge request timed out";
            error_response["error"] = error;
            response_cb_(error_response);
        }
        pending_.erase(id);
    }
}

// ---------------------------------------------------------------------------
// Synchronous command: send + brief poll loop
// ---------------------------------------------------------------------------

Dictionary RuntimeBridgeServer::send_command_sync(int instance_id,
                                                    const String &cmd,
                                                    const Dictionary &params,
                                                    int max_poll_ms) {
    auto it = instances_.find(instance_id);
    if (it == instances_.end()) {
        Dictionary r;
        r["ok"] = false;
        r["error"] = "Instance not found";
        return r;
    }

    const int64_t pending_id = next_request_id_++;
    GameInstance &inst = it->value;

    // Register pending so process_complete_messages can match it
    PendingRequest pr;
    pr.id = pending_id;
    pr.instance_id = instance_id;
    pr.deadline_msec = Time::get_singleton()->get_ticks_msec() + static_cast<uint64_t>(max_poll_ms);
    pr.sync = true;
    pending_[pending_id] = pr;

    // Build and send the frame
    Dictionary msg;
    msg["cmd"] = cmd;
    msg["params"] = params;
    msg["id"] = pending_id;

    String json_str = JSON::stringify(msg);
    PackedByteArray json_bytes = json_str.to_utf8_buffer();
    const uint32_t len = static_cast<uint32_t>(json_bytes.size());
    PackedByteArray frame;
    frame.resize(4 + json_bytes.size());
    auto *fp = frame.ptrw();
    fp[0] = static_cast<uint8_t>((len >> 24) & 0xFF);
    fp[1] = static_cast<uint8_t>((len >> 16) & 0xFF);
    fp[2] = static_cast<uint8_t>((len >> 8) & 0xFF);
    fp[3] = static_cast<uint8_t>(len & 0xFF);
    std::copy_n(json_bytes.ptr(), json_bytes.size(), fp + 4);

    Error err = inst.connection->put_data(frame);
    if (err != OK) {
        pending_.erase(pending_id);
        Dictionary r;
        r["ok"] = false;
        r["error"] = "Send failed";
        return r;
    }

    // Poll briefly, yielding to allow game process to respond.
    // Short timeout (max_poll_ms ≤ 100ms) — safe for fast operations.
    using namespace godot;
    const uint64_t deadline = pr.deadline_msec;
    Dictionary result;
    bool done = false;

    while (Time::get_singleton()->get_ticks_msec() < deadline) {
        // Let the OS schedule the game process
        OS::get_singleton()->delay_msec(1);

        inst.connection->poll();
        const int64_t avail = inst.connection->get_available_bytes();
        if (avail <= 0) continue;

        Array chunk = inst.connection->get_partial_data(static_cast<int>(avail));
        if (static_cast<Error>(static_cast<int>(chunk[0])) != OK) continue;
        inst.read_buf_.append_array(static_cast<PackedByteArray>(chunk[1]));

        // Let process_complete_messages handle frame parsing and matching
        process_complete_messages(instance_id, inst);

        // Check if our pending request was completed
        if (!pending_.has(pending_id)) {
            // Erased by process_complete_messages — response dispatched via callback
            done = true;
            break;
        }

        if (pending_[pending_id].completed) {
            result = pending_[pending_id].response;
            pending_.erase(pending_id);
            done = true;
            break;
        }
    }

    if (done) {
        return result;
    }

    // Not ready within budget — clean up sync pending and return empty
    pending_.erase(pending_id);
    return Dictionary();
}

// ---------------------------------------------------------------------------
// Error handling
// ---------------------------------------------------------------------------

void RuntimeBridgeServer::error_all_pending(const String &code, const String &msg) {
    if (!response_cb_) {
        // Keep sync requests — they'll be cleaned up by send_command_sync()
        Vector<int64_t> async_dead;
        for (const auto &kv : pending_) {
            if (!kv.value.sync) async_dead.push_back(kv.key);
        }
        for (int64_t id : async_dead) pending_.erase(id);
        return;
    }

    Vector<int64_t> dead;
    for (const auto &kv : pending_) {
        if (kv.value.sync) continue;  // sync requests handle their own errors
        dead.push_back(kv.key);
    }
    for (int64_t id : dead) {
        Dictionary error_response;
        error_response["id"] = id;
        error_response["instance_id"] = pending_[id].instance_id;
        Dictionary error;
        error["code"] = code;
        error["message"] = msg;
        error_response["error"] = error;
        response_cb_(error_response);
        pending_.erase(id);
    }
}

void RuntimeBridgeServer::error_instance_pending(int instance_id,
                                                   const String &code,
                                                   const String &msg) {
    if (!response_cb_) {
        Vector<int64_t> dead;
        for (const auto &kv : pending_) {
            if (!kv.value.sync && kv.value.instance_id == instance_id)
                dead.push_back(kv.key);
        }
        for (int64_t id : dead) pending_.erase(id);
        return;
    }

    Vector<int64_t> dead;
    for (const auto &kv : pending_) {
        if (kv.value.sync) continue;  // sync requests handle their own errors
        if (kv.value.instance_id == instance_id)
            dead.push_back(kv.key);
    }
    for (int64_t id : dead) {
        Dictionary error_response;
        error_response["id"] = id;
        error_response["instance_id"] = instance_id;
        Dictionary error;
        error["code"] = code;
        error["message"] = msg;
        error_response["error"] = error;
        response_cb_(error_response);
        pending_.erase(id);
    }
}

// ---------------------------------------------------------------------------
// Instance queries
// ---------------------------------------------------------------------------

Array RuntimeBridgeServer::get_connected_instances() const {
    Array result;
    const uint64_t now = Time::get_singleton()->get_ticks_msec();
    for (const auto &kv : instances_) {
        Dictionary entry;
        entry["id"] = kv.key;
        entry["connected_at"] = kv.value.connected_at;
        entry["uptime_msec"] = static_cast<int64_t>(now - kv.value.connected_at);
        result.push_back(entry);
    }
    return result;
}

int RuntimeBridgeServer::get_default_instance_id() const {
    if (instances_.is_empty()) return -1;
    int oldest_id = -1;
    uint64_t oldest_time = UINT64_MAX;
    for (const auto &kv : instances_) {
        if (kv.value.connected_at < oldest_time) {
            oldest_time = kv.value.connected_at;
            oldest_id = kv.key;
        }
    }
    return oldest_id;
}

// ---------------------------------------------------------------------------
// Bridge watcher (for WaitForBridgeTool)
// ---------------------------------------------------------------------------

void RuntimeBridgeServer::start_watcher(const Variant &jsonrpc_id, int timeout_ms) {
    watcher_.active = true;
    watcher_.jsonrpc_id = jsonrpc_id;
    watcher_.deadline_msec = Time::get_singleton()->get_ticks_msec() + timeout_ms;
    watcher_.handler = mcp_handler_;
}

void RuntimeBridgeServer::check_watcher() {
    if (!watcher_.active || !watcher_.handler) return;

    if (has_instances()) {
        watcher_.active = false;
        Dictionary result_data;
        result_data["message"] = "Bridge connected";
        result_data["instances"] = get_connected_instances();
        Dictionary content;
        content["type"] = "text";
        content["text"] = JSON::stringify(result_data);
        Array contents;
        contents.push_back(content);
        Dictionary result;
        result["content"] = contents;
        result["isError"] = false;
        Dictionary jsonrpc;
        jsonrpc["jsonrpc"] = "2.0";
        jsonrpc["id"] = watcher_.jsonrpc_id;
        jsonrpc["result"] = result;
        watcher_.handler->enqueue_event(jsonrpc);
        return;
    }

    if (Time::get_singleton()->get_ticks_msec() > watcher_.deadline_msec) {
        watcher_.active = false;
        Dictionary jsonrpc;
        jsonrpc["jsonrpc"] = "2.0";
        jsonrpc["id"] = watcher_.jsonrpc_id;
        Dictionary err;
        err["code"] = -32603;
        err["message"] = "No game instance connected within timeout";
        jsonrpc["error"] = err;
        watcher_.handler->enqueue_event(jsonrpc);
    }
}

// ---------------------------------------------------------------------------
// Response formatting
// ---------------------------------------------------------------------------

Dictionary RuntimeBridgeServer::make_response(const Dictionary &raw) {
    if (raw.has("pending")) return raw;
    if (raw.has("success")) {
        if (!raw["success"] && !raw.has("error")) {
            Dictionary error;
            error["code"] = String(error_codes::BRIDGE_ERROR);
            error["message"] = "Unknown error";
            Dictionary r;
            r["success"] = false;
            r["error"] = error;
            return r;
        }
        return raw;
    }

    if (!raw.has("ok") || !raw["ok"]) {
        Dictionary error;
        error["code"] = String(error_codes::BRIDGE_ERROR);
        error["message"] = raw.get("error", String("Command execution failed"));
        Dictionary r;
        r["success"] = false;
        r["error"] = error;
        return r;
    }
    Dictionary r;
    r["success"] = true;
    r["data"] = raw.get("data", Variant());
    return r;
}

} // namespace godot_mcp
