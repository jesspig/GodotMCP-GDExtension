#include "pipeline_runner_base.hpp"

#include "../logging.hpp"
#include "../server/registry/handler_registry.hpp"

#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>

#include <deque>
#include <set>
#include <vector>

namespace godot_mcp {
namespace pipeline {

using namespace godot;

namespace {

std::vector<size_t> topological_order(const std::vector<StepDef> &steps) {
    godot::HashMap<godot::String, size_t> id_to_idx;
    for (size_t i = 0; i < steps.size(); ++i) {
        id_to_idx[steps[i].id] = i;
    }
    std::vector<std::vector<size_t>> adj(steps.size());
    std::vector<int> in_deg(steps.size(), 0);
    for (size_t i = 0; i < steps.size(); ++i) {
        for (const auto &dep : steps[i].depends_on) {
            if (id_to_idx.has(dep)) {
                size_t dep_idx = id_to_idx[dep];
                adj[dep_idx].push_back(i);
                in_deg[i]++;
            }
        }
    }
    std::deque<size_t> q;
    for (size_t i = 0; i < steps.size(); ++i) {
        if (in_deg[i] == 0) q.push_back(i);
    }
    std::vector<size_t> sorted;
    while (!q.empty()) {
        size_t idx = q.front(); q.pop_front();
        sorted.push_back(idx);
        for (size_t next : adj[idx]) {
            if (--in_deg[next] == 0) q.push_back(next);
        }
    }
    std::set<size_t> in_sorted(sorted.begin(), sorted.end());
    for (size_t i = 0; i < steps.size(); ++i) {
        if (in_sorted.find(i) == in_sorted.end()) sorted.push_back(i);
    }
    return sorted;
}

} // anonymous namespace

PipelineRunnerBase::PipelineRunnerBase(HandlerRegistry *registry)
    : registry_(registry) {}

Dictionary PipelineRunnerBase::execute_chain(const std::vector<ChainStep> &chain,
                                              const char *chain_name,
                                              FailPolicy default_on_failure,
                                              CallStats &stats) {
    Dictionary result;
    result["error"] = String();
    result["skipped_count"] = 0;
    result["errors"] = Array();
    Array chain_errors;
    const int64_t chain_size = static_cast<int64_t>(chain.size());
    for (int64_t i = 0; i < chain_size; ++i) {
        const auto &step = chain[static_cast<size_t>(i)];
        if (step.tool.is_empty()) continue;
        const Dictionary exec_result = registry_->execute(step.tool, step.args);
        stats.call_count++;
        stats.unique_tools.insert(step.tool);
        bool step_failed = false;
        String err_msg;
        if (exec_result.has("error")) {
            const Variant err = exec_result["error"];
            err_msg = err.get_type() == Variant::DICTIONARY
                ? String(Dictionary(err).get("message", "Unknown error"))
                : String(err);
            step_failed = true;
        } else if (exec_result.has("success") && !exec_result["success"].operator bool()) {
            err_msg = String("returned success=false");
            step_failed = true;
        }
        if (step_failed) {
            stats.call_fail++;
            stats.unique_fail.insert(step.tool);
            const String full_err = String(chain_name) + String(" step '") + step.tool + String("' failed: ") + err_msg;
            Dictionary err_entry;
            err_entry["tool"] = step.tool;
            err_entry["error"] = full_err;
            chain_errors.push_back(err_entry);
            stats.errors.push_back(err_entry);
            FailPolicy fp = step.on_failure;
            if (fp == FailPolicy::FailFast) {
                const int64_t remaining = chain_size - i - 1;
                stats.call_skip += static_cast<int>(remaining);
                for (int64_t s = i + 1; s < chain_size; ++s) {
                    const auto &skipped_step = chain[static_cast<size_t>(s)];
                    if (!skipped_step.tool.is_empty()) stats.unique_skip.insert(skipped_step.tool);
                }
                result["error"] = full_err;
                result["skipped_count"] = static_cast<int>(remaining);
                result["errors"] = chain_errors;
                return result;
            } else if (fp == FailPolicy::Stop) {
                result["error"] = full_err;
                result["errors"] = chain_errors;
                return result;
            }
        } else {
            stats.call_success++;
            stats.unique_success.insert(step.tool);
        }
    }
    result["errors"] = chain_errors;
    return result;
}

Dictionary PipelineRunnerBase::execute_step(const StepDef &step_def,
                                              PipelineContext &ctx,
                                              CallStats &stats) {
    Dictionary step_result;
    step_result["tool"] = step_def.tool;
    step_result["description"] = step_def.description;
    step_result["step_id"] = step_def.id;
    const Dictionary expanded_args = ctx.expand_templates(step_def.args);
    int max_retries = step_def.retry_count.value_or(0);
    int delay_ms = step_def.retry_delay_ms.value_or(0);
    Dictionary raw_result;
    int attempts = 0;
    const int64_t start_us = Time::get_singleton()->get_ticks_usec();
    for (int attempt = 0; attempt <= max_retries; ++attempt) {
        if (attempt > 0 && delay_ms > 0) {
            uint64_t wait_until = Time::get_singleton()->get_ticks_msec() + delay_ms;
            while (Time::get_singleton()->get_ticks_msec() < wait_until) {}
        }
        attempts = attempt + 1;
        raw_result = registry_->execute(step_def.tool, expanded_args);
        if (!raw_result.has("error")) break;
        if (step_def.expect.has("status") && step_def.expect["status"] == "error") break;
    }
    const int64_t duration_us = Time::get_singleton()->get_ticks_usec() - start_us;
    step_result["raw_result"] = raw_result;
    step_result["attempts"] = attempts;
    step_result["duration_us"] = duration_us;
    stats.call_count++;
    stats.unique_tools.insert(step_def.tool);
    bool step_passed = false;
    String status_str;
    String final_error;
    bool has_error = raw_result.has("error");
    if (has_error) {
        if (step_def.allow_failure) {
            step_passed = true;
            status_str = "PASS";
        } else {
            stats.call_fail++;
            stats.unique_fail.insert(step_def.tool);
            final_error = String(raw_result["error"]);
            status_str = "FAIL";
            Dictionary err_entry;
            err_entry["tool"] = step_def.tool;
            err_entry["error"] = final_error;
            stats.errors.push_back(err_entry);
        }
    } else {
        stats.call_success++;
        stats.unique_success.insert(step_def.tool);
        step_passed = true;
        status_str = "PASS";
    }
    step_result["passed"] = step_passed;
    step_result["status"] = status_str;
    if (!final_error.is_empty()) step_result["error"] = final_error;
    return step_result;
}

std::vector<size_t> PipelineRunnerBase::topological_sort(const std::vector<StepDef> &steps) {
    return topological_order(steps);
}

PipelineRunnerBase::RunResult PipelineRunnerBase::run(const PipelineDef &pipeline, PipelineContext &ctx) {
    RunResult run_result;
    CallStats stats;
    // before_all
    if (!pipeline.before_all.empty()) {
        const Dictionary before_all_result = execute_chain(
            pipeline.before_all, "before_all", pipeline.on_failure, stats);
        if (!String(before_all_result["error"]).is_empty()) {
            run_result.pipeline_error = before_all_result["error"];
            for (const auto &stage : pipeline.stages) {
                for (const auto &step : stage.steps) {
                    StepRecord sr;
                    sr.tool = step.tool;
                    sr.step_id = step.id;
                    sr.status = "SKIP";
                    sr.error = before_all_result["error"];
                    run_result.steps.push_back(sr);
                    run_result.skipped++;
                }
            }
            run_result.total = run_result.steps.size();
            return run_result;
        }
    }
    int total = 0, passed = 0, failed = 0, skipped = 0;
    bool pipeline_stopped = false;
    for (const auto &stage : pipeline.stages) {
        for (const auto &step : stage.steps) { (void)step; total++; }
    }
    for (const auto &stage : pipeline.stages) {
        if (pipeline_stopped) {
            for (const auto &step : stage.steps) {
                skipped++;
                StepRecord sr;
                sr.tool = step.tool; sr.step_id = step.id; sr.status = "SKIP";
                run_result.steps.push_back(sr);
            }
            continue;
        }
        const auto sorted_indices = topological_order(stage.steps);
        bool stage_failed = false;
        for (size_t si : sorted_indices) {
            const auto &step = stage.steps[si];
            if (pipeline_stopped) {
                skipped++;
                StepRecord sr;
                sr.tool = step.tool; sr.step_id = step.id; sr.status = "SKIP";
                run_result.steps.push_back(sr);
                continue;
            }
            if (stage_failed && stage.on_failure == FailPolicy::FailFast) {
                skipped++;
                StepRecord sr;
                sr.tool = step.tool; sr.step_id = step.id; sr.status = "SKIP";
                run_result.steps.push_back(sr);
                continue;
            }
            if (step.when.has_value() && !ctx.eval_when(step.when.value())) {
                skipped++;
                StepRecord sr;
                sr.tool = step.tool; sr.step_id = step.id; sr.status = "SKIP";
                run_result.steps.push_back(sr);
                pipeline::StepResult ctx_result;
                ctx_result.status = StepStatus::Skipped;
                ctx.record_step(step.id, ctx_result);
                continue;
            }
            bool deps_satisfied = true;
            for (const auto &dep_id : step.depends_on) {
                auto dep_sr = ctx.get_step(dep_id);
                if (dep_sr.has_value() && dep_sr->status != StepStatus::Passed) {
                    deps_satisfied = false;
                    break;
                }
            }
            if (!deps_satisfied && !step.allow_failure) {
                skipped++;
                StepRecord sr;
                sr.tool = step.tool; sr.step_id = step.id; sr.status = "SKIP";
                sr.error = "Dependency not satisfied";
                run_result.steps.push_back(sr);
                pipeline::StepResult ctx_result;
                ctx_result.status = StepStatus::Skipped;
                ctx.record_step(step.id, ctx_result);
                continue;
            }
            // before_each
            if (!stage.before_each.empty()) {
                const Dictionary be_result = execute_chain(
                    stage.before_each, "before_each", stage.on_failure, stats);
                if (!String(be_result["error"]).is_empty()) {
                    failed++;
                    StepRecord sr;
                    sr.tool = step.tool; sr.step_id = step.id; sr.status = "FAIL";
                    sr.error = be_result["error"];
                    run_result.steps.push_back(sr);
                    pipeline::StepResult ctx_result;
                    ctx_result.status = StepStatus::Failed;
                    ctx.record_step(step.id, ctx_result);
                    continue;
                }
            }
            // execute step
            const Dictionary result = execute_step(step, ctx, stats);
            const String status = result.get("status", "");
            {
                pipeline::StepStatus ss = StepStatus::Passed;
                if (status == "FAIL") ss = StepStatus::Failed;
                else if (status == "SKIP") ss = StepStatus::Skipped;
                pipeline::StepResult ctx_result;
                ctx_result.status = ss;
                ctx_result.raw_result = result.get("raw_result", Dictionary());
                ctx.record_step(step.id, ctx_result);
            }
            // after_each
            if (!stage.after_each.empty()) {
                const Dictionary ae_result = execute_chain(
                    stage.after_each, "after_each", stage.on_failure, stats);
                if (!String(ae_result["error"]).is_empty()) {
                    log_warn("pipeline", String(ae_result["error"]));
                }
            }
            StepRecord sr;
            sr.tool = result.get("tool", "");
            sr.step_id = result.get("step_id", "");
            sr.duration_us = result.get("duration_us", (int64_t)0);
            sr.result = result.get("raw_result", Dictionary());
            sr.status = status;
            if (result.has("error")) sr.error = result["error"];
            if (status == "PASS") passed++;
            else if (status == "SKIP") skipped++;
            else failed++;
            run_result.steps.push_back(sr);
            if (status != "PASS") {
                FailPolicy fp = step.on_failure;
                if (fp == FailPolicy::Stop) pipeline_stopped = true;
                else if (fp == FailPolicy::FailFast) stage_failed = true;
            }
        }
    }
    // after_all
    if (!pipeline.after_all.empty()) {
        const Dictionary after_all_result = execute_chain(
            pipeline.after_all, "after_all", pipeline.on_failure, stats);
        if (!String(after_all_result["error"]).is_empty()) {
            log_warn("pipeline", String(after_all_result["error"]));
        }
    }
    run_result.total = total;
    run_result.passed = passed;
    run_result.failed = failed;
    run_result.skipped = skipped;
    return run_result;
}

} // namespace pipeline
} // namespace godot_mcp
