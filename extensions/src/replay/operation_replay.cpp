#include "replay/operation_replay.hpp"
#include "pipeline/yaml_parser.hpp"
#include "server/registry/handler_registry.hpp"

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>

namespace godot_mcp::replay {

using godot::Array;
using godot::Dictionary;
using godot::String;
using godot::Variant;

OperationReplay::ReplayResult OperationReplay::replay(const String &yaml_content) {
    ReplayResult result;

    Variant parsed = parse_yaml(yaml_content);
    if (parsed.get_type() != Variant::DICTIONARY) {
        result.error_message = "YAML top-level must be a dictionary";
        return result;
    }

    Dictionary dict = parsed;
    if (dict.has("error")) {
        result.error_message = String(dict["error"]);
        return result;
    }

    if (!dict.has("steps")) {
        result.error_message = "Missing required field 'steps' in YAML";
        return result;
    }

    Array steps = dict["steps"];
    return replay_steps(steps);
}

OperationReplay::ReplayResult OperationReplay::replay_steps(const Array &steps) {
    ReplayResult result;
    result.steps_total = static_cast<int>(steps.size());

    if (!registry_) {
        result.error_message = "HandlerRegistry not set";
        return result;
    }

    for (int64_t i = 0; i < steps.size(); i++) {
        Dictionary step = steps[i];

        if (!step.has("tool")) {
            result.error_message = "Step missing required field 'tool'";
            return result;
        }

        String tool_name = String(step["tool"]);
        Dictionary step_args;
        if (step.has("args")) {
            step_args = step["args"];
        }

        Dictionary exec_result = registry_->execute(tool_name, step_args);

        result.steps_executed++;

        if (!exec_result.get("success", false)) {
            if (exec_result.has("error")) {
                Dictionary err = exec_result["error"];
                result.error_message = String(err.get("message", "Step failed: " + tool_name));
            } else {
                result.error_message = "Step failed: " + tool_name;
            }
            return result;
        }
    }

    result.success = true;
    return result;
}

} // namespace godot_mcp::replay
