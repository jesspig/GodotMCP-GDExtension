// DEPRECATED — use bridge_server.hpp + bridge_server.cpp instead.
// Retained for reference only. Scheduled for removal in v0.4.0.
#include "bridge.hpp"
#include "built_in/cmd_utils/error_codes.hpp"
#include "logging.hpp"

#include <algorithm>
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>

using namespace godot;

namespace godot_mcp {

RuntimeBridge::RuntimeBridge() = default;
RuntimeBridge::~RuntimeBridge() = default;

void RuntimeBridge::poll() {
    if (status_ == CONNECTING) {
        tcp_->poll();
        auto s = tcp_->get_status();
        if (s == StreamPeerTCP::STATUS_CONNECTED) {
            status_ = CONNECTED;
            connecting_since_ = 0;
            tcp_->set_no_delay(true);
            log_info("runtime", String("Runtime bridge connected on :") + String::num_int64(port_));
        } else if (s == StreamPeerTCP::STATUS_ERROR) {
            status_ = DISCONNECTED;
            tcp_.unref();
            log_warn("runtime", "Runtime bridge connection failed");
        } else if (Time::get_singleton()->get_ticks_msec() - connecting_since_ > CONNECT_TIMEOUT_MS) {
            disconnect();
            log_warn("runtime", "Runtime bridge connect timed out");
        }
        return;
    }

    if (status_ == CONNECTED) {
        tcp_->poll();
        auto s = tcp_->get_status();
        if (s == StreamPeerTCP::STATUS_ERROR || s == StreamPeerTCP::STATUS_NONE) {
            status_ = DISCONNECTED;
            tcp_.unref();
            log_warn("runtime", "Runtime bridge disconnected");
        }
    }
}

void RuntimeBridge::connect() {
    if (status_ == CONNECTING || status_ == CONNECTED) return;

    tcp_.instantiate();
    Error err = tcp_->connect_to_host("127.0.0.1", port_);
    if (err != OK) {
        log_warn("runtime", String("Runtime bridge connect failed: err=") + String::num_int64(err));
        tcp_.unref();
        status_ = DISCONNECTED;
        return;
    }
    status_ = CONNECTING;
    connecting_since_ = Time::get_singleton()->get_ticks_msec();
    log_info("runtime", String("Connecting to runtime bridge 127.0.0.1:") + String::num_int64(port_));
}

void RuntimeBridge::disconnect() {
    // Calling get_status() or disconnect_from_host() on a socket whose
    // remote end already closed triggers Godot's internal precondition
    // check (_sock.is_null() || !_sock->is_open()) which prints a noisy
    // ERROR. Just unref — the OS will clean up the connection.
    tcp_.unref();
    status_ = DISCONNECTED;
    connecting_since_ = 0;
    log_info("runtime", "Runtime bridge disconnected");
}

Dictionary RuntimeBridge::send_command(const String &cmd, const Dictionary &params, int timeout_ms) {
    Dictionary raw;

    if (status_ != CONNECTED) {
        raw["ok"] = false;
        raw["error"] = "Game not running";
        return make_response(raw);
    }

    int64_t id = next_id_++;

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

    Error err = tcp_->put_data(frame);
    if (err != OK) {
        disconnect();
        raw["ok"] = false;
        raw["error"] = "Bridge connection broken";
        return make_response(raw);
    }

    raw = read_response(timeout_ms);
    if (raw.has("pending")) {
        return raw;
    }
    if (raw.has("id") && raw["id"].get_type() == Variant::INT && static_cast<int64_t>(raw["id"]) != id) {
        disconnect();
        Dictionary err_dict;
        err_dict["ok"] = false;
        err_dict["error"] = String("Response ID mismatch: expected ") + String::num_int64(id) + String(", got ") + String::num_int64(static_cast<int64_t>(raw["id"]));
        return make_response(err_dict);
    }
    return make_response(raw);
}

Dictionary RuntimeBridge::read_response(int timeout_ms) {
    uint32_t msg_length = 0;
    bool header_done = false;
    int body_received = 0;
    PackedByteArray body;

    const uint64_t start = Time::get_singleton()->get_ticks_msec();
    PackedByteArray header_buf;

    while (true) {
        const uint64_t elapsed = Time::get_singleton()->get_ticks_msec() - start;
        if (elapsed >= static_cast<uint64_t>(timeout_ms)) break;

        tcp_->poll();
        if (tcp_->get_status() != StreamPeerTCP::STATUS_CONNECTED) {
            disconnect();
            Dictionary err;
            err["ok"] = false;
            err["error"] = "Connection lost during read";
            return err;
        }

        const int64_t avail = tcp_->get_available_bytes();
        if (avail <= 0) {
            OS::get_singleton()->delay_msec(5);
            continue;
        }

        Array chunk = tcp_->get_partial_data(static_cast<int>(avail));
        if (static_cast<Error>(static_cast<int>(chunk[0])) != OK) continue;
        PackedByteArray data = chunk[1];

        if (!header_done) {
            for (int i = 0; i < data.size(); i++) {
                header_buf.push_back(data[i]);
                if (header_buf.size() == 4) {
                    msg_length = (static_cast<uint32_t>(header_buf[0]) << 24) |
                                 (static_cast<uint32_t>(header_buf[1]) << 16) |
                                 (static_cast<uint32_t>(header_buf[2]) << 8) |
                                  static_cast<uint32_t>(header_buf[3]);
                    if (msg_length == 0 || msg_length > 65536) {
                        disconnect();
                        Dictionary r;
                        r["ok"] = false;
                        r["error"] = "Invalid message length";
                        return r;
                    }
                    header_done = true;
                    PackedByteArray remaining;
                    int rem_count = data.size() - (i + 1);
                    if (rem_count > 0) {
                        remaining.resize(rem_count);
                        std::copy_n(data.ptr() + i + 1, rem_count, remaining.ptrw());
                        body.append_array(remaining);
                        body_received += rem_count;
                    }
                    break;
                }
            }
        } else {
            body.append_array(data);
            body_received += data.size();
        }

        if (header_done && body_received >= static_cast<int>(msg_length)) {
            String text;
            text.parse_utf8(reinterpret_cast<const char *>(body.ptr()), static_cast<int>(msg_length));
            Ref<JSON> json;
            json.instantiate();
            if (json->parse(text) == OK) {
                Variant result = json->get_data();
                if (result.get_type() == Variant::DICTIONARY) {
                    return result;
                }
            }
            disconnect();
            Dictionary r;
            r["ok"] = false;
            r["error"] = "Invalid JSON in response";
            return r;
        }
    }

    Dictionary r;
    r["pending"] = true;
    return r;
}

Dictionary RuntimeBridge::make_response(const Dictionary &raw) {
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
