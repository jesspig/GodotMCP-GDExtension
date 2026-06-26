#pragma once

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/templates/vector.hpp>

namespace godot_mcp::replay {

using godot::Array;
using godot::Dictionary;
using godot::String;
using godot::Vector;

struct RecordedOp {
    String tool_name;
    Dictionary args;
    double timestamp;
};

class OperationRecorder {
public:
    void start_recording();
    void stop_recording();
    bool is_recording() const { return recording_; }
    void clear();

    void record_tool_call(const String &tool_name, const Dictionary &args);
    String get_yaml() const;

    const Vector<RecordedOp> &operations() const { return operations_; }

private:
    bool recording_ = false;
    Vector<RecordedOp> operations_;
};

OperationRecorder &get_global_recorder();

} // namespace godot_mcp::replay
