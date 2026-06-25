#include "pipeline_parser.hpp"

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>

using namespace godot;

#include <deque>
#include <set>
#include <vector>

namespace godot_mcp {
namespace pipeline {

namespace {

FailPolicy parse_fail_policy(const godot::String &s, FailPolicy default_val) {
    if (s == "fail_fast") return FailPolicy::FailFast;
    if (s == "stop") return FailPolicy::Stop;
    if (s == "continue") return FailPolicy::Continue;
    return default_val;
}

ChainStep parse_chain_step(const godot::Dictionary &d) {
    ChainStep cs;
    cs.tool = d.get("tool", "");
    cs.args = d.get("args", godot::Dictionary());
    if (d.has("on_failure")) {
        cs.on_failure = parse_fail_policy(d["on_failure"], FailPolicy::Stop);
    }
    return cs;
}

std::vector<ChainStep> parse_chain_array(const godot::Array &arr) {
    std::vector<ChainStep> result;
    const int64_t n = static_cast<int64_t>(arr.size());
    result.reserve(static_cast<size_t>(n));
    for (int64_t i = 0; i < n; ++i) {
        result.push_back(parse_chain_step(arr[i]));
    }
    return result;
}

ParseError make_err(const godot::String &msg, const godot::String &field) {
    ParseError e;
    e.message = msg;
    e.field = field;
    return e;
}

} // anonymous namespace

ParseResult parse_pipeline(const godot::Dictionary &config) {
    auto pipeline = std::make_shared<PipelineDef>();
    pipeline->name = config.get("name", "");
    pipeline->description = config.get("description", "");
    pipeline->headless = config.get("headless", true);
    if (!config.has("pipeline")) {
        return make_err("Missing required key 'pipeline'", "pipeline");
    }
    if (config["pipeline"].get_type() != godot::Variant::DICTIONARY) {
        return make_err("'pipeline' must be a dictionary", "pipeline");
    }
    const godot::Dictionary pipe = config["pipeline"];
    pipeline->on_failure = parse_fail_policy(pipe.get("on_failure", ""), FailPolicy::FailFast);
    if (pipe.has("before_all")) pipeline->before_all = parse_chain_array(pipe["before_all"]);
    if (pipe.has("after_all")) pipeline->after_all = parse_chain_array(pipe["after_all"]);
    if (!pipe.has("stages")) return make_err("Missing required key 'pipeline.stages'", "stages");
    if (pipe["stages"].get_type() != godot::Variant::ARRAY) return make_err("'pipeline.stages' must be an array", "stages");
    const godot::Array stages_arr = pipe["stages"];
    std::set<godot::String> all_step_ids;
    for (int64_t si = 0; si < static_cast<int64_t>(stages_arr.size()); ++si) {
        if (stages_arr[si].get_type() != godot::Variant::DICTIONARY)
            return make_err(godot::String("stages[") + godot::String::num_int64(si) + "] must be a dictionary", godot::String("stages[") + godot::String::num_int64(si) + "]");
        const godot::Dictionary stage_dict = stages_arr[si];
        StageDef stage;
        stage.id = stage_dict.get("id", "");
        if (stage.id.is_empty()) return make_err(godot::String("stages[") + godot::String::num_int64(si) + "].id is required", godot::String("stages[") + godot::String::num_int64(si) + "].id");
        stage.name = stage_dict.get("name", stage.id);
        if (stage_dict.has("before_each")) stage.before_each = parse_chain_array(stage_dict["before_each"]);
        if (stage_dict.has("after_each")) stage.after_each = parse_chain_array(stage_dict["after_each"]);
        stage.on_failure = parse_fail_policy(stage_dict.get("on_failure", ""), pipeline->on_failure);
        if (!stage_dict.has("steps")) return make_err(godot::String("stage '") + stage.id + "' missing 'steps'", godot::String("stages[") + godot::String::num_int64(si) + "].steps");
        if (stage_dict["steps"].get_type() != godot::Variant::ARRAY) return make_err(godot::String("stage '") + stage.id + "'.steps must be an array", "");
        const godot::Array steps_arr = stage_dict["steps"];
        std::vector<StepDef> parsed_steps;
        const int64_t n_steps = static_cast<int64_t>(steps_arr.size());
        parsed_steps.reserve(static_cast<size_t>(n_steps));
        for (int64_t j = 0; j < n_steps; ++j) {
            if (steps_arr[j].get_type() != godot::Variant::DICTIONARY) return make_err(godot::String("step in stage '") + stage.id + "' must be a dictionary", "");
            const godot::Dictionary step_dict = steps_arr[j];
            StepDef step;
            step.id = step_dict.get("id", "");
            if (step.id.is_empty()) return make_err(godot::String("step in stage '") + stage.id + "' missing 'id'", "");
            if (!all_step_ids.insert(step.id).second) return make_err(godot::String("Duplicate step id '") + step.id + "'", step.id);
            step.tool = step_dict.get("tool", "");
            if (step.tool.is_empty()) return make_err(godot::String("step '") + step.id + "' missing 'tool'", step.id + ".tool");
            step.description = step_dict.get("description", "");
            step.args = step_dict.get("args", godot::Dictionary());
            step.expect = step_dict.get("expect", godot::Dictionary());
            step.disk_verify = step_dict.get("disk_verify", godot::Dictionary());
            if (step_dict.has("depends_on")) {
                const godot::Array deps = step_dict["depends_on"];
                const int64_t nd = static_cast<int64_t>(deps.size());
                step.depends_on.reserve(static_cast<size_t>(nd));
                for (int64_t k = 0; k < nd; ++k) step.depends_on.push_back(deps[k]);
            }
            if (step_dict.has("when")) {
                WhenClause wc;
                const godot::Dictionary wd = step_dict["when"];
                wc.step_id = wd.get("step", "");
                wc.field = wd.get("status", "passed");
                wc.expected = true;
                step.when = wc;
            }
            step.on_failure = parse_fail_policy(step_dict.get("on_failure", ""), stage.on_failure);
            if (step_dict.has("retry")) {
                const godot::Dictionary rd = step_dict["retry"];
                step.retry_count = static_cast<int>(static_cast<int64_t>(rd.get("count", 0)));
                step.retry_delay_ms = static_cast<int>(static_cast<int64_t>(rd.get("delay_ms", 0)));
            }
            step.allow_failure = step_dict.get("allow_failure", false);
            parsed_steps.push_back(std::move(step));
        }
        std::set<godot::String> stage_step_ids;
        for (const auto &s : parsed_steps) stage_step_ids.insert(s.id);
        for (const auto &s : parsed_steps) {
            for (const auto &dep : s.depends_on) {
                if (stage_step_ids.find(dep) == stage_step_ids.end())
                    return make_err(godot::String("step '") + s.id + "' depends_on unknown '" + dep + "' in stage '" + stage.id + "'", godot::String(s.id) + godot::String(".depends_on"));
            }
        }
        {
            godot::HashMap<godot::String, std::vector<godot::String>> adj;
            godot::HashMap<godot::String, int> in_deg;
            for (const auto &s : parsed_steps) {
                if (!in_deg.has(s.id)) in_deg[s.id] = 0;
                for (const auto &dep : s.depends_on) {
                    adj[dep].push_back(s.id);
                    int d = in_deg.has(s.id) ? in_deg[s.id] : 0;
                    in_deg[s.id] = d + 1;
                }
            }
            std::deque<godot::String> q;
            for (const auto &kv : in_deg) { if (kv.value == 0) q.push_back(kv.key); }
            int visited = 0;
            while (!q.empty()) {
                godot::String id = q.front(); q.pop_front(); visited++;
                if (adj.has(id)) {
                    for (const auto &next : adj[id]) {
                        int d = in_deg[next];
                        if (d > 0) { d--; in_deg[next] = d; if (d == 0) q.push_back(next); }
                    }
                }
            }
            if (visited != static_cast<int>(parsed_steps.size()))
                return make_err(godot::String("Cycle detected in stage '") + stage.id + "' depends_on", godot::String("stages[") + godot::String::num_int64(si) + "].steps");
        }
        stage.steps = std::move(parsed_steps);
        pipeline->stages.push_back(std::move(stage));
    }
    return std::shared_ptr<PipelineDef>(std::move(pipeline));
}

} // namespace pipeline
} // namespace godot_mcp
