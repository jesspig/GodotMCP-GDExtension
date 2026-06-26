#include "workflow_runner.hpp"

#include "../server/registry/handler_registry.hpp"

#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>

using namespace godot;

namespace godot_mcp {
namespace pipeline {

WorkflowRunner::WorkflowRunner(HandlerRegistry *registry)
    : PipelineRunnerBase(registry) {}

Dictionary WorkflowRunner::run_workflow(const PipelineDef &pipeline,
                                         const Dictionary &vars,
                                         int timeout_ms,
                                         int max_steps) {
    Dictionary output;
    const int64_t start_us = Time::get_singleton()->get_ticks_usec();

    // Count total steps
    int step_count = 0;
    for (const auto &stage : pipeline.stages) {
        step_count += static_cast<int>(stage.steps.size());
    }

    // Cap max steps
    if (step_count > max_steps) {
        output["error"] = String("Workflow exceeds max_steps limit (") +
                          String::num_int64(max_steps) + String(")");
        output["succeeded"] = 0;
        output["failed"] = 0;
        output["total_steps"] = step_count;
        output["duration_ms"] = 0;
        output["timed_out"] = false;
        output["steps"] = Array();
        return output;
    }

    PipelineContext ctx;
    ctx.set_workflow_vars(vars);

    RunResult result = PipelineRunnerBase::run(pipeline, ctx);

    const int64_t duration_us = Time::get_singleton()->get_ticks_usec() - start_us;
    bool timed_out = (duration_us / 1000) > static_cast<int64_t>(timeout_ms);

    output["total_steps"] = result.total;
    output["succeeded"] = result.passed;
    output["failed"] = result.failed;
    output["duration_ms"] = duration_us / 1000;
    output["timed_out"] = timed_out;

    if (!result.pipeline_error.is_empty()) {
        output["pipeline_error"] = result.pipeline_error;
    }

    Array steps_result;
    for (const auto &sr : result.steps) {
        Dictionary entry;
        entry["id"] = sr.step_id;
        entry["tool"] = sr.tool;
        entry["status"] = sr.status;
        entry["result"] = sr.result;
        if (!sr.error.is_empty()) entry["error"] = sr.error;
        steps_result.push_back(entry);
    }
    output["steps"] = steps_result;

    return output;
}

} // namespace pipeline
} // namespace godot_mcp
