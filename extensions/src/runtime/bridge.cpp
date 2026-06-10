#include "bridge.hpp"
#include "logging.hpp"

#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/classes/os.hpp>
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
            log_info("runtime", String("Runtime bridge connected on :") + String::num_int64(port_));
        } else if (s == StreamPeerTCP::STATUS_ERROR) {
            status_ = DISCONNECTED;
            tcp_.unref();
            log_warn("runtime", "Runtime bridge connection failed");
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
    tcp_->set_no_delay(true);
    Error err = tcp_->connect_to_host("127.0.0.1", port_);
    if (err != OK) {
        log_warn("runtime", String("Runtime bridge connect failed: err=") + String::num_int64(err));
        tcp_.unref();
        status_ = DISCONNECTED;
        return;
    }
    status_ = CONNECTING;
    log_info("runtime", String("Connecting to runtime bridge 127.0.0.1:") + String::num_int64(port_));
}

void RuntimeBridge::disconnect() {
    if (tcp_.is_valid()) {
        tcp_->disconnect_from_host();
        tcp_.unref();
    }
    status_ = DISCONNECTED;
    log_info("runtime", "Runtime bridge disconnected");
}

Dictionary RuntimeBridge::send_command(const String &cmd, const Dictionary &params, int timeout_ms) {
    if (status_ != CONNECTED) {
        Dictionary r;
        r["ok"] = false;
        r["error"] = String::utf8("游戏未运行");
        return r;
    }

    int id = next_id_++;

    Dictionary msg;
    msg["cmd"] = cmd;
    msg["params"] = params;
    msg["id"] = id;

    String json_str = JSON::stringify(msg);
    PackedByteArray data = json_str.to_utf8_buffer();

    Error err = tcp_->put_data(data);
    if (err != OK) {
        disconnect();
        Dictionary r;
        r["ok"] = false;
        r["error"] = String::utf8("桥接连接断开");
        return r;
    }

    return read_response(timeout_ms);
}

Dictionary RuntimeBridge::read_response(int timeout_ms) {
    int elapsed = 0;
    const int step = 50;
    PackedByteArray buf;

    while (elapsed < timeout_ms) {
        tcp_->poll();
        if (tcp_->get_available_bytes() > 0) {
            Array chunk = tcp_->get_partial_data(tcp_->get_available_bytes());
            if ((int)chunk[0] == OK) {
                PackedByteArray data = chunk[1];
                buf.append_array(data);
            }
        }

        if (buf.size() > 0) {
            String text = buf.get_string_from_utf8();
            Ref<JSON> json;
            json.instantiate();
            Error parse_err = json->parse(text);
            if (parse_err == OK) {
                Variant result = json->get_data();
                if (result.get_type() == Variant::DICTIONARY) {
                    return result;
                }
            }
        }

        OS::get_singleton()->delay_msec(step);
        elapsed += step;
    }

    Dictionary r;
    r["ok"] = false;
    r["error"] = String("Runtime bridge command timed out (") + String::num_int64(timeout_ms) + String("ms)");
    return r;
}

Dictionary RuntimeBridge::make_response(const Dictionary &raw) {
    if (!raw.has("ok") || !raw["ok"]) {
        Dictionary error;
        error["code"] = "BRIDGE_ERROR";
        error["message"] = raw.get("error", String::utf8("命令执行失败"));
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
