#pragma once

#include <cstdint>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/variant.hpp>

namespace godot_mcp {

struct PendingDestructiveOp {
    godot::Variant jsonrpc_id;
    godot::String tool_name;
    godot::Dictionary arguments;
    uint64_t created_msec = 0;

    enum class State : uint8_t { Pending, Resolved };
    State state = State::Pending;
};

} // namespace godot_mcp
