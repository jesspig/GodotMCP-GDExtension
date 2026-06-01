// lsp/client.hpp — LSP validation client using StreamPeerTCP

#pragma once

#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/classes/stream_peer.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot_mcp {

class LspClient {
public:
    godot::Dictionary validate(const godot::String &path,
                               const godot::String &source,
                               const godot::String &file_uri,
                               const godot::String &root_uri,
                               uint16_t port = 6005,
                               int timeout_ms = 5000);
};

} // namespace godot_mcp
