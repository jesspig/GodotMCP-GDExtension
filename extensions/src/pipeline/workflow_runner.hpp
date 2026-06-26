#pragma once

#include "pipeline_runner_base.hpp"

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace godot_mcp {
namespace pipeline {

class WorkflowRunner : public PipelineRunnerBase {
public:
    WorkflowRunner(HandlerRegistry *registry);

    Dictionary run_workflow(const PipelineDef &pipeline,
                            const Dictionary &vars,
                            int timeout_ms = 30000,
                            int max_steps = 50);
};

} // namespace pipeline
} // namespace godot_mcp
