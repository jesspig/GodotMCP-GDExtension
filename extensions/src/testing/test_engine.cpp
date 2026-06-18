#include "test_engine.hpp"
#include "pipeline_parser.hpp"
#include "yaml_parser.hpp"

#include "../logging.hpp"
#include "../server/registry/handler_registry.hpp"

#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>

using namespace godot;

namespace godot_mcp {

TestEngine::TestEngine(HandlerRegistry *registry) {
    runner_ = std::make_unique<pipeline::PipelineRunner>(registry);
}

godot::Dictionary TestEngine::run(const godot::String &yaml_content) {
    // Parse YAML to Godot Dictionary
    const godot::Variant parsed = parse_yaml(yaml_content);
    if (parsed.get_type() != godot::Variant::DICTIONARY) {
        godot::Dictionary err;
        err["error"] = godot::String("YAML top-level must be a dictionary");
        return err;
    }
    const godot::Dictionary config = parsed;

    // Parse pipeline config
    const auto parse_result = pipeline::parse_pipeline(config);

    // Check parse result
    if (std::holds_alternative<pipeline::ParseError>(parse_result)) {
        const auto &err = std::get<pipeline::ParseError>(parse_result);
        godot::Dictionary err_dict;
        err_dict["error"] = godot::String("Parse error: ") + err.message;
        log_error("test_engine", godot::String("Parse error [") + err.field + "]: " + err.message);
        return err_dict;
    }

    // Run the pipeline
    const auto &pipeline = std::get<std::shared_ptr<pipeline::PipelineDef>>(parse_result);
    return runner_->run(pipeline);
}

} // namespace godot_mcp
