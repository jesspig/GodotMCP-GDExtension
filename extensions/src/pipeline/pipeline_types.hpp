#pragma once

#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>

#include <functional>
#include <memory>
#include <optional>
#include <variant>
#include <vector>

namespace godot_mcp {
namespace pipeline {

enum class FailPolicy { Continue, Stop, FailFast };
enum class StepStatus { Pending, Running, Passed, Failed, Skipped };

struct StepError {
    godot::String code;
    godot::String message;
};

struct ExecResult {
    godot::Dictionary raw;
    std::optional<StepError> error;
    int64_t duration_us = 0;
};

struct StepResult {
    StepStatus status = StepStatus::Pending;
    std::optional<ExecResult> exec;
    godot::Dictionary raw_result;
    std::optional<StepError> error;
    int64_t duration_us = 0;
    int retry_attempts = 0;
};

struct WhenClause {
    godot::String step_id;
    godot::String field;
    bool expected = true;
};

struct ChainStep {
    godot::String tool;
    godot::Dictionary args;
    FailPolicy on_failure = FailPolicy::Stop;
};

struct StepDef {
    godot::String id;
    godot::String tool;
    godot::String description;
    godot::Dictionary args;
    godot::Dictionary expect;
    godot::Dictionary disk_verify;
    std::vector<godot::String> depends_on;
    std::optional<WhenClause> when;
    FailPolicy on_failure = FailPolicy::Continue;
    std::optional<int> retry_count;
    std::optional<int> retry_delay_ms;
    bool allow_failure = false;
};

struct StageDef {
    godot::String id;
    godot::String name;
    std::vector<StepDef> steps;
    std::vector<ChainStep> before_each;
    std::vector<ChainStep> after_each;
    FailPolicy on_failure = FailPolicy::FailFast;
};

struct PipelineDef {
    godot::String name;
    godot::String description;
    bool headless = true;
    std::vector<ChainStep> before_all;
    std::vector<ChainStep> after_all;
    std::vector<StageDef> stages;
    FailPolicy on_failure = FailPolicy::FailFast;
};

struct ParseError {
    godot::String message;
    godot::String field;
};

using ParseResult = std::variant<std::shared_ptr<PipelineDef>, ParseError>;

} // namespace pipeline
} // namespace godot_mcp
