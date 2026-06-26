#pragma once

#include "pipeline_types.hpp"

#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot_mcp {
namespace pipeline {

// TestParser: parses test YAML format (has pipeline: wrapper, supports expect/disk_verify)
// Uses the existing parse_pipeline() internally
struct TestParseResult {
    std::shared_ptr<PipelineDef> pipeline;
    std::optional<ParseError> error;
};

class TestParser {
public:
    static TestParseResult parse(const godot::Dictionary &config);
    static TestParseResult parse_yaml(const godot::String &yaml_text);
};

} // namespace pipeline
} // namespace godot_mcp
