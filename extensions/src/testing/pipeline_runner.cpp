#include "pipeline_runner.hpp"

#include "test_assertions.hpp"
#include "godot_file_verifier.hpp"

#include "../logging.hpp"
#include "../server/registry/handler_registry.hpp"

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/editor_file_system.hpp>
#include <godot_cpp/classes/editor_file_system_directory.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>

#include <deque>
#include <set>
#include <vector>

using namespace godot;

namespace godot_mcp {
namespace pipeline {

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
        if (in_sorted.find(i) == in_sorted.end()) {
            sorted.push_back(i);
        }
    }

    return sorted;
}

void collect_res_paths(const godot::Variant &v, godot::Array &out) {
    switch (v.get_type()) {
        case godot::Variant::STRING: {
            const godot::String s = v;
            if (s.begins_with("res://")) {
                out.push_back(s);
            }
            break;
        }
        case godot::Variant::DICTIONARY: {
            const godot::Dictionary d = v;
            const godot::Array keys = d.keys();
            for (int i = 0; i < keys.size(); ++i) {
                collect_res_paths(d[keys[i]], out);
            }
            break;
        }
        case godot::Variant::ARRAY: {
            const godot::Array a = v;
            for (int i = 0; i < a.size(); ++i) {
                collect_res_paths(a[i], out);
            }
            break;
        }
        default:
            break;
    }
}

} // anonymous namespace

PipelineRunner::PipelineRunner(HandlerRegistry *registry)
    : registry_(registry) {}

PipelineRunner::FileSnapshot PipelineRunner::take_snapshot() {
    FileSnapshot snap;
    godot::EditorInterface *ei = godot::EditorInterface::get_singleton();
    if (!ei) return snap;

    godot::EditorFileSystem *efs = ei->get_resource_filesystem();
    if (!efs) return snap;

    efs->scan();

    godot::EditorFileSystemDirectory *root = efs->get_filesystem();
    if (!root) return snap;

    std::deque<godot::EditorFileSystemDirectory *> queue;
    queue.push_back(root);

    godot::PackedStringArray all_paths;
    while (!queue.empty()) {
        godot::EditorFileSystemDirectory *dir = queue[0];
        queue.pop_front();
        if (!dir) continue;

        for (int i = 0; i < dir->get_file_count(); ++i) {
            all_paths.push_back(dir->get_file_path(i));
        }
        for (int i = 0; i < dir->get_subdir_count(); ++i) {
            queue.push_back(dir->get_subdir(i));
        }
    }

    snap.paths = all_paths;
    return snap;
}

godot::Array PipelineRunner::track_paths(const godot::Dictionary &result) {
    godot::Array paths;
    collect_res_paths(result, paths);
    return paths;
}

void PipelineRunner::cleanup(const godot::Array &tracked,
                              const FileSnapshot &before,
                              const FileSnapshot &after,
                              godot::Array &out_deleted,
                              godot::Array &out_skipped) {
    godot::HashMap<godot::String, bool> before_set;
    for (int i = 0; i < before.paths.size(); ++i) {
        before_set[before.paths[i]] = true;
    }

    godot::HashMap<godot::String, bool> after_set;
    for (int i = 0; i < after.paths.size(); ++i) {
        after_set[after.paths[i]] = true;
    }

    std::vector<godot::String> new_files;
    for (const auto &kv : after_set) {
        if (!before_set.has(kv.key)) {
            new_files.push_back(kv.key);
        }
    }

    godot::HashMap<godot::String, bool> tracked_set;
    for (int i = 0; i < tracked.size(); ++i) {
        tracked_set[godot::String(tracked[i])] = true;
    }

    for (const godot::String &path : new_files) {
        if (!tracked_set.has(path)) {
            out_skipped.push_back(path);
            continue;
        }

        if (path == "res://" || !path.begins_with("res://")) {
            out_skipped.push_back(path);
            continue;
        }

        godot::Ref<godot::DirAccess> da = godot::DirAccess::open("res://");
        if (da.is_valid()) {
            const godot::Error err = da->remove(path);
            if (err == godot::OK) {
                out_deleted.push_back(path);
            } else {
                log_warn("test_engine", godot::String("Failed to delete: ") + path);
                out_skipped.push_back(path);
            }
        }

        godot::String dir_path = path.get_base_dir();
        for (int hop = 0; hop < 64 && dir_path != "res://" && dir_path.begins_with("res://"); ++hop) {
            godot::Ref<godot::DirAccess> d = godot::DirAccess::open(dir_path);
            if (d.is_null()) break;

            bool empty = true;
            d->list_dir_begin();
            godot::String n = d->get_next();
            while (!n.is_empty()) {
                if (n != "." && n != "..") { empty = false; break; }
                n = d->get_next();
            }
            d->list_dir_end();

            if (empty) {
                godot::Ref<godot::DirAccess> parent = godot::DirAccess::open(dir_path.get_base_dir());
                if (parent.is_valid()) {
                    const godot::String dir_name = dir_path.get_file();
                    const godot::Error rm_err = parent->remove(dir_name);
                    if (rm_err != godot::OK) {
                        log_warn("test_engine",
                                 godot::String("Failed to remove empty dir: ") + dir_path);
                        break;
                    }
                } else {
                    break;
                }
            } else {
                break;
            }
            dir_path = dir_path.get_base_dir();
        }
    }
}

void PipelineRunner::record_setting_before_call(const godot::String &tool_name,
                                                  const godot::Dictionary &args) {
    if (tool_name != "set_setting" && tool_name != "reset_setting")
        return;
    const godot::String path = args.get("setting_path", "");
    if (path.is_empty() || settings_snapshot_.has(path))
        return;

    auto *ps = godot::ProjectSettings::get_singleton();
    if (ps && ps->has_setting(path)) {
        settings_snapshot_[path] = ps->get_setting(path);
    } else {
        settings_snapshot_[path] = godot::Variant();
    }
}

void PipelineRunner::restore_settings() {
    if (settings_snapshot_.size() == 0)
        return;
    auto *ps = godot::ProjectSettings::get_singleton();
    if (!ps)
        return;

    const godot::Array keys = settings_snapshot_.keys();
    for (int i = 0; i < keys.size(); ++i) {
        const godot::String key = keys[i];
        const godot::Variant val = settings_snapshot_[key];
        if (val.get_type() == godot::Variant::NIL) {
            if (ps->has_setting(key))
                ps->clear(key);
        } else {
            ps->set_setting(key, val);
        }
    }
    settings_snapshot_.clear();
}

void PipelineRunner::backup_project_godot() {
    godot::Ref<godot::FileAccess> fa = godot::FileAccess::open("res://project.godot", godot::FileAccess::READ);
    if (fa.is_null()) return;
    project_godot_backup_ = fa->get_as_text();
    fa->close();
}

void PipelineRunner::restore_project_godot() {
    if (project_godot_backup_.is_empty()) return;

    godot::Ref<godot::FileAccess> fa = godot::FileAccess::open("res://project.godot", godot::FileAccess::WRITE);
    if (fa.is_null()) {
        log_warn("test_engine", "Cannot write project.godot for restoration");
        return;
    }
    fa->store_string(project_godot_backup_);
    fa->close();
    project_godot_backup_ = godot::String();

    godot::EditorInterface *ei = godot::EditorInterface::get_singleton();
    if (ei) {
        godot::EditorFileSystem *efs = ei->get_resource_filesystem();
        if (efs) efs->scan();
    }
}

godot::Dictionary PipelineRunner::execute_chain(const std::vector<ChainStep> &chain,
                                                  const char *chain_name,
                                                  FailPolicy default_on_failure,
                                                  CallStats &stats) {
    godot::Dictionary result;
    result["error"] = godot::String();
    result["skipped_count"] = 0;
    result["errors"] = godot::Array();

    godot::Array chain_errors;
    const int64_t chain_size = static_cast<int64_t>(chain.size());

    for (int64_t i = 0; i < chain_size; ++i) {
        const auto &step = chain[static_cast<size_t>(i)];
        if (step.tool.is_empty()) continue;

        record_setting_before_call(step.tool, step.args);
        const godot::Dictionary exec_result = registry_->execute(step.tool, step.args);

        stats.call_count++;
        stats.unique_tools.insert(step.tool);

        bool step_failed = false;
        godot::String err_msg;

        if (exec_result.has("error")) {
            const godot::Variant err = exec_result["error"];
            err_msg = err.get_type() == godot::Variant::DICTIONARY
                ? godot::String(godot::Dictionary(err).get("message", "Unknown error"))
                : godot::String(err);
            step_failed = true;
        } else if (exec_result.has("success") && !exec_result["success"].operator bool()) {
            err_msg = godot::String("returned success=false");
            step_failed = true;
        }

        if (step_failed) {
            stats.call_fail++;
            stats.unique_fail.insert(step.tool);

            const godot::String full_err = godot::String(chain_name) + godot::String(" step '") + step.tool + godot::String("' failed: ") + err_msg;

            godot::Dictionary err_entry;
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
                    if (!skipped_step.tool.is_empty()) {
                        stats.unique_skip.insert(skipped_step.tool);
                    }
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

godot::Dictionary PipelineRunner::execute_step(const StepDef &step_def,
                                                 PipelineContext &ctx,
                                                 CallStats &stats) {
    godot::Dictionary test_result;
    test_result["tool"] = step_def.tool;
    test_result["description"] = step_def.description;
    test_result["step_id"] = step_def.id;

    const godot::Dictionary expanded_args = ctx.expand_templates(step_def.args);

    int max_retries = step_def.retry_count.value_or(0);
    int delay_ms = step_def.retry_delay_ms.value_or(0);

    godot::Dictionary raw_result;
    int attempts = 0;
    const int64_t start_us = godot::Time::get_singleton()->get_ticks_usec();

    for (int attempt = 0; attempt <= max_retries; ++attempt) {
        if (attempt > 0 && delay_ms > 0) {
            uint64_t wait_until = godot::Time::get_singleton()->get_ticks_msec() + delay_ms;
            while (godot::Time::get_singleton()->get_ticks_msec() < wait_until) {
                // busy-wait on main thread
            }
        }

        attempts = attempt + 1;
        record_setting_before_call(step_def.tool, expanded_args);
        raw_result = registry_->execute(step_def.tool, expanded_args);

        // Stop retrying if tool returned without error
        if (!raw_result.has("error")) {
            break;
        }
        // If tool has error but expect block allows error, accept the result
        if (step_def.expect.has("status") && step_def.expect["status"] == "error") {
            break;
        }
    }

    const int64_t duration_us = godot::Time::get_singleton()->get_ticks_usec() - start_us;

    test_result["raw_result"] = raw_result;
    test_result["tracked_paths"] = track_paths(raw_result);
    test_result["attempts"] = attempts;
    test_result["duration_us"] = duration_us;

    stats.call_count++;
    stats.unique_tools.insert(step_def.tool);

    const godot::String assert_error = run_assertions(step_def.expect, raw_result);

    bool step_passed = false;
    godot::String status_str;
    godot::String final_error;

    if (!assert_error.is_empty()) {
        // Assertions failed
        if (step_def.allow_failure) {
            step_passed = true;
            status_str = "PASS";
        } else {
            stats.call_fail++;
            stats.unique_fail.insert(step_def.tool);
            final_error = assert_error;
            status_str = "FAIL";

            godot::Dictionary err_entry;
            err_entry["tool"] = step_def.tool;
            err_entry["error"] = assert_error;
            stats.errors.push_back(err_entry);
        }
    } else {
        // Assertions passed — run disk verification
        bool disk_fail = false;
        if (!step_def.disk_verify.is_empty()) {
            const godot::Array disk_errors = run_disk_verification(step_def.disk_verify);
            if (disk_errors.size() > 0) {
                const godot::String disk_err = godot::String("Disk verification failed: ") + godot::JSON::stringify(disk_errors);
                if (step_def.allow_failure) {
                    log_warn("test_engine", disk_err);
                } else {
                    stats.call_fail++;
                    stats.unique_fail.insert(step_def.tool);
                    step_passed = false;
                    status_str = "FAIL";
                    final_error = disk_err;
                    disk_fail = true;

                    godot::Dictionary err_entry;
                    err_entry["tool"] = step_def.tool;
                    err_entry["error"] = disk_err;
                    stats.errors.push_back(err_entry);
                }
            }
        }
        if (!disk_fail) {
            stats.call_success++;
            stats.unique_success.insert(step_def.tool);
            step_passed = true;
            status_str = "PASS";
        }
    }

    test_result["passed"] = step_passed;
    test_result["status"] = status_str;
    if (!final_error.is_empty()) {
        test_result["error"] = final_error;
    }

    return test_result;
}

godot::Dictionary PipelineRunner::run(const std::shared_ptr<PipelineDef> &pipeline) {
    godot::Dictionary suite_result;
    suite_result["name"] = pipeline->name;
    suite_result["description"] = pipeline->description;

    const int64_t start_time_us = godot::Time::get_singleton()->get_ticks_usec();
    CallStats stats;
    pipeline::StepResult ctx_result;

    backup_project_godot();
    const FileSnapshot before = take_snapshot();

    // before_all
    if (!pipeline->before_all.empty()) {
        const godot::Dictionary before_all_result = execute_chain(
            pipeline->before_all, "before_all", pipeline->on_failure, stats);
        if (!godot::String(before_all_result["error"]).is_empty()) {
            godot::Array test_results;
            int total = 0;
            for (const auto &stage : pipeline->stages) {
                for (const auto &step : stage.steps) {
                    total++;
                    stats.call_skip++;
                    godot::Dictionary entry;
                    entry["tool"] = step.tool;
                    entry["description"] = step.description;
                    entry["step_id"] = step.id;
                    entry["passed"] = false;
                    entry["status"] = "SKIP";
                    entry["error"] = before_all_result["error"];
                    test_results.push_back(entry);
                }
            }

            restore_settings();
            restore_project_godot();
            const FileSnapshot after = take_snapshot();
            const int64_t end_us = godot::Time::get_singleton()->get_ticks_usec();

            godot::Dictionary summary;
            summary["total"] = total;
            summary["passed"] = 0;
            summary["failed"] = 0;
            summary["skipped"] = total;
            summary["call_count"] = stats.call_count;
            summary["call_success"] = stats.call_success;
            summary["call_fail"] = stats.call_fail;
            summary["call_skip"] = stats.call_skip;
            summary["duration_ms"] = (end_us - start_time_us) / 1000;
            summary["errors"] = before_all_result["errors"];
            summary["cleanup_deleted"] = godot::Array();
            summary["cleanup_skipped"] = godot::Array();

            suite_result["summary"] = summary;
            suite_result["tests"] = test_results;
            return suite_result;
        }
    }

    PipelineContext ctx;
    godot::Array test_results;
    int total = 0, passed = 0, failed = 0, skipped = 0;
    bool pipeline_stopped = false;

    for (const auto &stage : pipeline->stages) {
        for (const auto &step : stage.steps) {
            total++;
        }
    }

    for (const auto &stage : pipeline->stages) {
        if (pipeline_stopped) {
            for (const auto &step : stage.steps) {
                skipped++;
                stats.call_skip++;
                godot::Dictionary entry;
                entry["tool"] = step.tool;
                entry["description"] = step.description;
                entry["step_id"] = step.id;
                entry["passed"] = false;
                entry["status"] = "SKIP";
                entry["stage"] = stage.id;
                test_results.push_back(entry);
            }
            continue;
        }

        const auto sorted_indices = topological_order(stage.steps);
        bool stage_failed = false;

        for (size_t si : sorted_indices) {
            const auto &step = stage.steps[si];

            if (pipeline_stopped) {
                skipped++; stats.call_skip++;
                godot::Dictionary entry;
                entry["tool"] = step.tool; entry["description"] = step.description;
                entry["step_id"] = step.id; entry["passed"] = false;
                entry["status"] = "SKIP"; entry["stage"] = stage.id;
                test_results.push_back(entry);
                continue;
            }

            if (stage_failed && stage.on_failure == FailPolicy::FailFast) {
                skipped++; stats.call_skip++;
                godot::Dictionary entry;
                entry["tool"] = step.tool; entry["description"] = step.description;
                entry["step_id"] = step.id; entry["passed"] = false;
                entry["status"] = "SKIP"; entry["stage"] = stage.id;
                test_results.push_back(entry);
                continue;
            }

            // when condition
            if (step.when.has_value() && !ctx.eval_when(step.when.value())) {
                skipped++; stats.call_skip++;
                godot::Dictionary entry;
                entry["tool"] = step.tool; entry["description"] = step.description;
                entry["step_id"] = step.id; entry["passed"] = false;
                entry["status"] = "SKIP"; entry["stage"] = stage.id;
                test_results.push_back(entry);
                ctx_result.status = StepStatus::Skipped;
                ctx.record_step(step.id, ctx_result);
                continue;
            }

            // dependency check
            bool deps_satisfied = true;
            for (const auto &dep_id : step.depends_on) {
                auto dep_sr = ctx.get_step(dep_id);
                if (dep_sr.has_value() && dep_sr->status != StepStatus::Passed) {
                    deps_satisfied = false;
                    break;
                }
            }
            if (!deps_satisfied && !step.allow_failure) {
                skipped++; stats.call_skip++;
                godot::Dictionary entry;
                entry["tool"] = step.tool; entry["description"] = step.description;
                entry["step_id"] = step.id; entry["passed"] = false;
                entry["status"] = "SKIP"; entry["stage"] = stage.id;
                entry["error"] = godot::String("Dependency not satisfied");
                test_results.push_back(entry);
                ctx_result.status = StepStatus::Skipped;
                ctx.record_step(step.id, ctx_result);
                continue;
            }

            // before_each
            if (!stage.before_each.empty()) {
                const godot::Dictionary be_result = execute_chain(
                    stage.before_each, "before_each", stage.on_failure, stats);
                if (!godot::String(be_result["error"]).is_empty()) {
                    failed++;
                    godot::Dictionary entry;
                    entry["tool"] = step.tool; entry["description"] = step.description;
                    entry["step_id"] = step.id; entry["passed"] = false;
                    entry["status"] = "FAIL"; entry["error"] = be_result["error"];
                    entry["stage"] = stage.id;
                    test_results.push_back(entry);
                    ctx_result.status = StepStatus::Failed;
                    ctx.record_step(step.id, ctx_result);
                    continue;
                }
            }

            // execute step
            const godot::Dictionary result = execute_step(step, ctx, stats);
            const godot::String status = result.get("status", "");

            {
                pipeline::StepStatus ss = StepStatus::Passed;
                if (status == "FAIL") ss = StepStatus::Failed;
                else if (status == "SKIP") ss = StepStatus::Skipped;
                ctx_result.status = ss;
                ctx_result.raw_result = result.get("raw_result", godot::Dictionary());
                ctx.record_step(step.id, ctx_result);
            }

            // collect tracked paths
            if (result.has("tracked_paths")) {
                const godot::Array paths = result["tracked_paths"];
                if (!suite_result.has("_all_tracked")) {
                    suite_result["_all_tracked"] = godot::Array();
                }
                godot::Array all_tracked = suite_result["_all_tracked"];
                const int old_size = static_cast<int>(all_tracked.size());
                all_tracked.resize(old_size + paths.size());
                for (int p = 0; p < paths.size(); ++p) {
                    all_tracked[old_size + p] = paths[p];
                }
                suite_result["_all_tracked"] = all_tracked;
            }

            // after_each
            if (!stage.after_each.empty()) {
                const godot::Dictionary ae_result = execute_chain(
                    stage.after_each, "after_each", stage.on_failure, stats);
                if (!godot::String(ae_result["error"]).is_empty()) {
                    log_warn("test_engine", godot::String(ae_result["error"]));
                }
            }

            godot::Dictionary entry;
            entry["tool"] = result.get("tool", "");
            entry["description"] = result.get("description", "");
            entry["step_id"] = result.get("step_id", "");
            entry["passed"] = result.get("passed", false);
            entry["status"] = result.get("status", "PASS");
            entry["stage"] = stage.id;
            if (result.has("duration_us")) {
                entry["duration_ms"] = static_cast<int64_t>(result["duration_us"]) / 1000;
            }
            if (result.has("error")) {
                entry["error"] = result["error"];
            }

            if (status == "PASS") passed++;
            else if (status == "SKIP") skipped++;
            else failed++;

            test_results.push_back(entry);

            if (status != "PASS") {
                FailPolicy fp = step.on_failure;
                if (fp == FailPolicy::Stop) {
                    pipeline_stopped = true;
                } else if (fp == FailPolicy::FailFast) {
                    stage_failed = true;
                }
            }
        }
    }

    // after_all
    godot::String teardown_error;
    bool teardown_failed = false;
    if (!pipeline->after_all.empty()) {
        const godot::Dictionary after_all_result = execute_chain(
            pipeline->after_all, "after_all", pipeline->on_failure, stats);
        if (!godot::String(after_all_result["error"]).is_empty()) {
            log_warn("test_engine", godot::String(after_all_result["error"]));
            teardown_failed = true;
            teardown_error = after_all_result["error"];
        }
    }

    restore_settings();
    restore_project_godot();

    const FileSnapshot after = take_snapshot();
    godot::Array all_tracked = suite_result.get("_all_tracked", godot::Array());
    godot::Array deleted, skipped_cleanup;
    cleanup(all_tracked, before, after, deleted, skipped_cleanup);

    const int64_t end_us = godot::Time::get_singleton()->get_ticks_usec();

    godot::Dictionary summary;
    summary["total"] = total;
    summary["passed"] = passed;
    summary["failed"] = failed;
    summary["skipped"] = skipped;
    summary["call_count"] = stats.call_count;
    summary["call_success"] = stats.call_success;
    summary["call_fail"] = stats.call_fail;
    summary["call_skip"] = stats.call_skip;

    {
        godot::PackedStringArray ut;
        for (const auto &s : stats.unique_tools) ut.push_back(s);
        summary["unique_tools"] = ut;
    }
    {
        godot::PackedStringArray us;
        for (const auto &s : stats.unique_success) us.push_back(s);
        summary["unique_success"] = us;
    }
    {
        godot::PackedStringArray uf;
        for (const auto &s : stats.unique_fail) uf.push_back(s);
        summary["unique_fail"] = uf;
    }
    {
        godot::PackedStringArray usk;
        for (const auto &s : stats.unique_skip) usk.push_back(s);
        summary["unique_skip"] = usk;
    }

    summary["duration_ms"] = (end_us - start_time_us) / 1000;
    summary["errors"] = stats.errors;
    summary["cleanup_deleted"] = deleted;
    summary["cleanup_skipped"] = skipped_cleanup;
    if (teardown_failed) {
        summary["teardown_failed"] = true;
        summary["teardown_error"] = teardown_error;
    }

    suite_result["summary"] = summary;
    suite_result["tests"] = test_results;

    if (suite_result.has("_all_tracked")) {
        suite_result.erase("_all_tracked");
    }

    return suite_result;
}

} // namespace pipeline
} // namespace godot_mcp
