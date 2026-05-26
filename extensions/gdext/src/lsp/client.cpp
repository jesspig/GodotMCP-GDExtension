// lsp/client.cpp — LSP validation client using StreamPeerTCP

#include "client.hpp"

#include "../logging.hpp"

#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/stream_peer.hpp>
#include <godot_cpp/classes/stream_peer_tcp.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>

using namespace godot;

namespace godot_mcp {
namespace {

// --- File-scope helpers (no Ref in headers) ---

bool send_message(Ref<StreamPeer> peer, const String &json_str) {
    PackedByteArray body = json_str.to_utf8_buffer();
    String header = String("Content-Length: ") + String::num_int64(body.size()) + String("\r\n\r\n");
    PackedByteArray header_bytes = header.to_utf8_buffer();
    PackedByteArray wire;
    wire.append_array(header_bytes);
    wire.append_array(body);
    return peer->put_data(wire) == OK;
}

String read_exactly(Ref<StreamPeerTCP> peer, int n, int timeout_ms) {
    int elapsed = 0; const int step = 50;
    PackedByteArray result;
    while (result.size() < n && elapsed < timeout_ms) {
        peer->poll();
        if (peer->get_available_bytes() > 0) {
            Array chunk = peer->get_partial_data(n - result.size());
            if ((int)chunk[0] == OK) { PackedByteArray data = chunk[1]; result.append_array(data); }
        }
        if (result.size() < n) { OS::get_singleton()->delay_msec(step); elapsed += step; }
    }
    return result.size() < n ? String() : result.get_string_from_utf8();
}

String read_until(Ref<StreamPeerTCP> peer, const String &delim, int timeout_ms) {
    int elapsed = 0; const int step = 50;
    PackedByteArray buf; PackedByteArray delim_bytes = delim.to_utf8_buffer();
    while (elapsed < timeout_ms) {
        peer->poll();
        if (peer->get_available_bytes() > 0) {
            Array chunk = peer->get_partial_data(peer->get_available_bytes());
            if ((int)chunk[0] == OK) {
                PackedByteArray data = chunk[1]; buf.append_array(data);
                if (buf.size() >= delim_bytes.size()) {
                    bool found = true;
                    for (int i = 0; i < delim_bytes.size(); i++) {
                        if (buf[buf.size() - delim_bytes.size() + i] != delim_bytes[i]) { found = false; break; }
                    }
                    if (found) {
                        int end = buf.size() - delim_bytes.size();
                        PackedByteArray result; result.resize(end);
                        for (int i = 0; i < end; i++) result[i] = buf[i];
                        return result.get_string_from_utf8();
                    }
                }
            }
        }
        OS::get_singleton()->delay_msec(step); elapsed += step;
    }
    return String();
}

String read_message(Ref<StreamPeerTCP> peer, int timeout_ms) {
    String header_part = read_until(peer, "\r\n\r\n", timeout_ms);
    if (header_part.is_empty()) return String();
    PackedStringArray lines = header_part.split("\r\n");
    int64_t content_length = 0;
    for (int i = 0; i < lines.size(); i++) {
        if (lines[i].begins_with("Content-Length:")) {
            content_length = lines[i].substr(String("Content-Length:").length()).strip_edges().to_int();
            break;
        }
    }
    return content_length <= 0 ? String() : read_exactly(peer, (int)content_length, timeout_ms);
}

} // namespace

Dictionary LspClient::validate(const String &path,
                                const String &source,
                                const String &file_uri,
                                const String &root_uri,
                                uint16_t port,
                                int timeout_ms) {
    // --- Connect -------------------------------------------------------
    Ref<StreamPeerTCP> tcp;
    tcp.instantiate();
    tcp->set_no_delay(true);
    Error err = tcp->connect_to_host("127.0.0.1", port);
    if (err != OK) { Dictionary r; r["ok"] = false; r["error"] = String("LSP connect failed: ") + String::num_int64(err); return r; }

    int elapsed = 0; const int step = 50;
    while (elapsed < timeout_ms) {
        tcp->poll();
        auto status = tcp->get_status();
        if (status == StreamPeerSocket::STATUS_CONNECTED) break;
        if (status == StreamPeerSocket::STATUS_ERROR) { Dictionary r; r["ok"] = false; r["error"] = "LSP connection error"; return r; }
        OS::get_singleton()->delay_msec(step); elapsed += step;
    }
    if (tcp->get_status() != StreamPeerSocket::STATUS_CONNECTED) { Dictionary r; r["ok"] = false; r["error"] = "LSP connection timed out"; return r; }

    // --- Initialize ----------------------------------------------------
    String init_req = String("{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\",\"params\":{") +
                      String("\"processId\":null,\"rootUri\":\"") + root_uri + String("\",\"capabilities\":{}}}");
    if (!send_message(tcp, init_req)) { tcp->disconnect_from_host(); Dictionary r; r["ok"] = false; r["error"] = "Failed to send initialize"; return r; }
    String init_resp = read_message(tcp, timeout_ms);
    if (init_resp.is_empty()) { tcp->disconnect_from_host(); Dictionary r; r["ok"] = false; r["error"] = "No response to initialize"; return r; }

    // --- initialized notification + didOpen ----------------------------
    send_message(tcp, String("{\"jsonrpc\":\"2.0\",\"method\":\"initialized\",\"params\":{}}"));
    String open_notif = String("{\"jsonrpc\":\"2.0\",\"method\":\"textDocument/didOpen\",\"params\":{\"textDocument\":{") +
                        String("\"uri\":\"") + file_uri + String("\",\"languageId\":\"gdscript\",\"version\":1,\"text\":") +
                        JSON::stringify(source) + String("}}}");
    send_message(tcp, open_notif);

    // --- Read diagnostics ----------------------------------------------
    Array diagnostics;
    int elapsed_read = 0; const int step_read = 50;
    while (elapsed_read < timeout_ms) {
        tcp->poll();
        if (tcp->get_available_bytes() <= 0) { OS::get_singleton()->delay_msec(step_read); elapsed_read += step_read; continue; }
        String msg = read_message(tcp, 1000);
        if (msg.is_empty()) continue;
        Ref<JSON> json; json.instantiate();
        if (json->parse(msg) != OK) continue;
        Variant root_v = json->get_data();
        if (root_v.get_type() != Variant::DICTIONARY) continue;
        Dictionary msg_dict = root_v;
        if (msg_dict.has("method") && (String)msg_dict["method"] == "textDocument/publishDiagnostics") {
            Dictionary params = msg_dict["params"];
            Array diags = params.get("diagnostics", Array());
            for (int i = 0; i < diags.size(); i++) {
                Dictionary d = diags[i];
                Dictionary range = d.get("range", Dictionary());
                Dictionary start = range.get("start", Dictionary());
                Dictionary entry;
                entry["line"] = start.get("line", 0);
                entry["column"] = start.get("character", 0);
                entry["severity"] = d.get("severity", 1);
                entry["message"] = d.get("message", "");
                entry["source"] = d.get("source", "gdscript");
                diagnostics.append(entry);
            }
        }
        if (diagnostics.size() > 0 && tcp->get_available_bytes() == 0) break;
    }

    // --- Shutdown ------------------------------------------------------
    send_message(tcp, String("{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"shutdown\",\"params\":{}}"));
    send_message(tcp, String("{\"jsonrpc\":\"2.0\",\"method\":\"exit\",\"params\":{}}"));
    tcp->disconnect_from_host();

    Dictionary r;
    r["ok"] = diagnostics.size() == 0;
    r["diagnostics"] = diagnostics;
    return r;
}

} // namespace godot_mcp