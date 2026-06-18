#pragma once

#include "../testing/pipeline_runner.hpp"

#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>

#include <memory>

namespace godot_mcp {

class HandlerRegistry;

class TestEngine {
public:
    TestEngine(HandlerRegistry *registry);

    godot::Dictionary run(const godot::String &yaml_content);

private:
    std::unique_ptr<pipeline::PipelineRunner> runner_;
};

} // namespace godot_mcp
