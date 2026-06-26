#include "workflow_parser.hpp"
#include "pipeline_parser.hpp"
#include "yaml_parser.hpp"

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>

#include <cstdlib>

using namespace godot;

namespace godot_mcp {
namespace pipeline {

// avoid conflict with pipeline_parser.cpp's make_err in Unity build
namespace {

ParseError wf_make_err(const String &msg, const String &field) {
    ParseError e;
    e.message = msg;
    e.field = field;
    return e;
}

// Parse time string like "30s", "5000ms", "1m" -> milliseconds
int parse_timeout(const String &s, int default_ms) {
    if (s.is_empty()) return default_ms;
    String lower = s.strip_edges().to_lower();
    int multiplier = 1000;
    if (lower.ends_with("ms")) {
        multiplier = 1;
        lower = lower.substr(0, lower.length() - 2);
    } else if (lower.ends_with("s")) {
        multiplier = 1000;
        lower = lower.substr(0, lower.length() - 1);
    } else if (lower.ends_with("m")) {
        multiplier = 60000;
        lower = lower.substr(0, lower.length() - 1);
    }
    
    int val = static_cast<int>(lower.to_int());
    
    return val * multiplier;
}

} // anonymous namespace

WorkflowParseResult WorkflowParser::parse(const Dictionary &input) {
    WorkflowParseResult result;

    if (!input.has("stages")) {
        result.error = wf_make_err("Missing required key 'stages'", "stages");
        return result;
    }

    // Build a config dict that parse_pipeline can understand
    Dictionary config;
    config["name"] = input.get("name", "workflow");
    config["description"] = input.get("description", "MCP workflow");
    config["headless"] = input.get("headless", false);

    Dictionary pipe;
    pipe["stages"] = input["stages"];

    // Reject test-only fields
    if (input.has("expect")) {
        result.error = wf_make_err("'expect' is not valid in workflow (use test YAML format)", "expect");
        return result;
    }
    if (input.has("disk_verify")) {
        result.error = wf_make_err("'disk_verify' is not valid in workflow", "disk_verify");
        return result;
    }

    if (input.has("on_failure")) {
        pipe["on_failure"] = input["on_failure"];
    }
    if (input.has("before_all")) {
        pipe["before_all"] = input["before_all"];
    }
    if (input.has("after_all")) {
        pipe["after_all"] = input["after_all"];
    }

    config["pipeline"] = pipe;

    auto parse_result = parse_pipeline(config);
    if (std::holds_alternative<ParseError>(parse_result)) {
        result.error = std::get<ParseError>(parse_result);
        return result;
    }

    result.pipeline = std::get<std::shared_ptr<PipelineDef>>(parse_result);
    result.pipeline->headless = input.get("headless", false);

    // Workflow-specific fields
    result.vars = input.get("vars", Dictionary());
    String timeout_str = input.get("timeout", "30s");
    result.timeout_ms = parse_timeout(timeout_str, 30000);
    result.max_steps = static_cast<int>(static_cast<int64_t>(input.get("max_steps", (int64_t)50)));

    return result;
}

WorkflowParseResult WorkflowParser::parse_yaml(const String &yaml_text) {
    Variant parsed = ::godot_mcp::parse_yaml(yaml_text);
    if (parsed.get_type() != Variant::DICTIONARY) {
        WorkflowParseResult result;
        result.error = wf_make_err("YAML top-level must be a dictionary", "yaml");
        return result;
    }
    Dictionary dict = parsed;
    if (dict.has("error")) {
        WorkflowParseResult result;
        result.error = wf_make_err(String(dict["error"]), "yaml");
        return result;
    }
    return parse(dict);
}

} // namespace pipeline
} // namespace godot_mcp
