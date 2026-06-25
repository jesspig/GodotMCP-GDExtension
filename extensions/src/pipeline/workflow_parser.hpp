#pragma once

#include "pipeline_types.hpp"

#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot_mcp {
namespace pipeline {

// WorkflowParser: parses workflow JSON/YAML input (no pipeline: wrapper, no expect/disk_verify)
// Supports: vars, timeout, max_steps
// Rejects: expect, disk_verify, allow_failure, headless
struct WorkflowParseResult {
    std::shared_ptr<PipelineDef> pipeline;
    godot::Dictionary vars;
    int timeout_ms = 30000;
    int max_steps = 50;
    std::optional<ParseError> error;
};

class WorkflowParser {
public:
    static WorkflowParseResult parse(const godot::Dictionary &input);
    static WorkflowParseResult parse_yaml(const godot::String &yaml_text);
};

} // namespace pipeline
} // namespace godot_mcp
