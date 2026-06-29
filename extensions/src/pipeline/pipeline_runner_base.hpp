#pragma once

#include "pipeline_context.hpp"
#include "pipeline_types.hpp"

#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>

using godot::Array;
using godot::Dictionary;
using godot::PackedStringArray;
using godot::String;

#include <memory>
#include <set>
#include <vector>

namespace godot_mcp {
class HandlerRegistry;
}

namespace godot_mcp {
namespace pipeline {

class PipelineRunnerBase {
public:
    struct StepRecord {
        godot::String tool;
        godot::String step_id;
        godot::String status;
        godot::Dictionary result;
        godot::String error;
        int64_t duration_us = 0;
        int attempts = 0;
    };

    struct RunResult {
        int total = 0, passed = 0, failed = 0, skipped = 0;
        godot::Vector<StepRecord> steps;
        godot::String pipeline_error;
    };

    PipelineRunnerBase(HandlerRegistry *registry);
    virtual ~PipelineRunnerBase() = default;

    RunResult run(const PipelineDef &pipeline, PipelineContext &ctx);

protected:
    struct CallStats {
        int call_count = 0;
        int call_success = 0;
        int call_fail = 0;
        int call_skip = 0;
        std::set<godot::String> unique_tools;
        std::set<godot::String> unique_success;
        std::set<godot::String> unique_fail;
        std::set<godot::String> unique_skip;
        godot::Array errors;
    };

    virtual Dictionary execute_step(const StepDef &step_def, PipelineContext &ctx, CallStats &stats);

    Dictionary execute_chain(const std::vector<ChainStep> &chain,
                              const char *chain_name,
                              FailPolicy default_on_failure,
                              CallStats &stats);

    static std::vector<size_t> topological_sort(const std::vector<StepDef> &steps);

    HandlerRegistry *registry_;
    CallStats stats_;
};

} // namespace pipeline
} // namespace godot_mcp