#include "replay/replay_tools.hpp"
#include "replay/operation_recorder.hpp"
#include "replay/operation_replay.hpp"
#include "built_in/cmd_utils/schema_builder.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>

using namespace godot;

namespace godot_mcp {

Dictionary RecordOperationsTool::build_input_schema() const {
    return SchemaBuilder()
        .prop("action", "string",
            "start to begin recording, stop to end and get YAML")
        .required(Array::make("action"))
        .build();
}

Dictionary RecordOperationsTool::execute_impl(const ToolContext &ctx) {
    String action = args_string(ctx.args, "action");

    if (action == "start") {
        replay::get_global_recorder().start_recording();
        Dictionary data;
        data["recording"] = true;
        return ToolResult::ok(data);
    }

    if (action == "stop") {
        replay::get_global_recorder().stop_recording();
        String yaml = replay::get_global_recorder().get_yaml();
        int64_t count = replay::get_global_recorder().operations().size();
        Dictionary data;
        data["recording"] = false;
        data["yaml"] = yaml;
        data["operation_count"] = count;
        return ToolResult::ok(data);
    }

    return ToolResult::err("INVALID_ACTION",
        "action must be 'start' or 'stop', got: " + action);
}

Dictionary ReplayOperationsTool::build_input_schema() const {
    return SchemaBuilder()
        .prop("yaml", "string",
            "YAML content to replay")
        .required(Array::make("yaml"))
        .build();
}

Dictionary ReplayOperationsTool::execute_impl(const ToolContext &ctx) {
    if (!registry_) {
        return ToolResult::err("NO_REGISTRY", "HandlerRegistry not available");
    }

    String yaml = args_string(ctx.args, "yaml");
    if (yaml.is_empty()) {
        return ToolResult::err("MISSING_ARG", "yaml cannot be empty");
    }

    replay::OperationReplay replay;
    replay.set_handler_registry(registry_);
    auto result = replay.replay(yaml);

    if (!result.success) {
        return ToolResult::err("REPLAY_FAILED", result.error_message);
    }

    Dictionary data;
    data["steps_executed"] = static_cast<int64_t>(result.steps_executed);
    data["steps_total"] = static_cast<int64_t>(result.steps_total);
    return ToolResult::ok(data);
}

} // namespace godot_mcp
