#pragma once

#include "pipeline_types.hpp"

#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>

#include <mutex>

namespace godot_mcp {
namespace pipeline {

class PipelineContext {
    mutable std::mutex mtx_;
    godot::HashMap<godot::String, StepResult> step_results_;
    godot::Dictionary workflow_vars_;

public:
    void record_step(const godot::String &id, const StepResult &r);
    std::optional<StepResult> get_step(const godot::String &id) const;
    bool eval_when(const WhenClause &c) const;
    godot::Dictionary expand_templates(const godot::Dictionary &args) const;

    void set_workflow_vars(const godot::Dictionary &vars) { workflow_vars_ = vars; }
    const godot::Dictionary &get_workflow_vars() const { return workflow_vars_; }
};

} // namespace pipeline
} // namespace godot_mcp
