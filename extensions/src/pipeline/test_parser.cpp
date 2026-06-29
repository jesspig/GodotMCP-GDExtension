#include "test_parser.hpp"
#include "pipeline_parser.hpp"
#include "yaml_parser.hpp"

#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>

using namespace godot;

namespace godot_mcp {
namespace pipeline {

TestParseResult TestParser::parse(const Dictionary &config) {
    TestParseResult result;
    auto parse_result = parse_pipeline(config);
    if (std::holds_alternative<ParseError>(parse_result)) {
        result.error = std::get<ParseError>(parse_result);
        return result;
    }
    result.pipeline = std::get<std::shared_ptr<PipelineDef>>(parse_result);
    return result;
}

TestParseResult TestParser::parse_yaml(const String &yaml_text) {
    Variant parsed = ::godot_mcp::parse_yaml(yaml_text);
    if (parsed.get_type() != Variant::DICTIONARY) {
        TestParseResult result;
        ParseError err;
        err.message = "YAML top-level must be a dictionary";
        err.field = "yaml";
        result.error = err;
        return result;
    }
    Dictionary dict = parsed;
    if (dict.has("error")) {
        TestParseResult result;
        ParseError err;
        err.message = String(dict["error"]);
        err.field = "yaml";
        result.error = err;
        return result;
    }
    return parse(dict);
}

} // namespace pipeline
} // namespace godot_mcp
