#include "replay/operation_recorder.hpp"

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>

namespace godot_mcp::replay {

using godot::Array;
using godot::Dictionary;
using godot::String;
using godot::Variant;

namespace {

String fmt_val(const Variant &v) {
    switch (v.get_type()) {
        case Variant::STRING:
            return "\"" + String(v) + "\"";
        case Variant::INT:
            return String::num(static_cast<int64_t>(v));
        case Variant::FLOAT:
            return String::num(static_cast<double>(v));
        case Variant::BOOL:
            return static_cast<bool>(v) ? "true" : "false";
        default:
            return "\"" + String(v) + "\"";
    }
}

}

OperationRecorder &get_global_recorder() {
    static OperationRecorder instance;
    return instance;
}

void OperationRecorder::start_recording() {
    recording_ = true;
    operations_.clear();
}

void OperationRecorder::stop_recording() {
    recording_ = false;
}

void OperationRecorder::clear() {
    operations_.clear();
    recording_ = false;
}

void OperationRecorder::record_tool_call(const String &tool_name, const Dictionary &args) {
    if (!recording_) return;
    RecordedOp op;
    op.tool_name = tool_name;
    op.args = args;
    op.timestamp = 0.0;
    operations_.push_back(op);
}

String OperationRecorder::get_yaml() const {
    String yaml = "name: recorded_workflow\nsteps:\n";
    for (int64_t i = 0; i < operations_.size(); i++) {
        const auto &op = operations_[i];
        yaml += "  - tool: " + op.tool_name + "\n";
        if (op.args.size() > 0) {
            yaml += "    args:\n";
            Array keys = op.args.keys();
            for (int64_t j = 0; j < keys.size(); j++) {
                String key = keys[j];
                yaml += "      " + key + ": " + fmt_val(op.args[key]) + "\n";
            }
        }
    }
    return yaml;
}

} // namespace godot_mcp::replay
