#pragma once

#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/array.hpp>

namespace godot_mcp {

// Forward declaration of HandlerRegistry (defined in godot_mcp namespace)
class HandlerRegistry;

namespace replay {

using godot::Array;
using godot::Dictionary;
using godot::String;

class OperationReplay {
public:
    struct ReplayResult {
        bool success = false;
        int steps_executed = 0;
        int steps_total = 0;
        String error_message;
    };

    ReplayResult replay(const String &yaml_content);
    ReplayResult replay_steps(const Array &steps);

    void set_handler_registry(HandlerRegistry *reg) { registry_ = reg; }

private:
    HandlerRegistry *registry_ = nullptr;
};

} // namespace replay
} // namespace godot_mcp
