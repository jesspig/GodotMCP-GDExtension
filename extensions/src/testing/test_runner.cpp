#include "test_runner.hpp"
#include "test_assertions.hpp"
#include "godot_file_verifier.hpp"

#include "../logging.hpp"
#include "../server/registry/handler_registry.hpp"

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/editor_file_system.hpp>
#include <godot_cpp/classes/editor_file_system_directory.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
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

namespace godot_mcp {
namespace pipeline {

using namespace godot;

namespace {

void collect_res_paths(const Variant &v, Array &out) {
    switch (v.get_type()) {
        case Variant::STRING: {
            const String s = v;
            if (s.begins_with("res://")) out.push_back(s);
            break;
        }
        case Variant::DICTIONARY: {
            const Dictionary d = v;
            const Array keys = d.keys();
            for (int i = 0; i < keys.size(); ++i) collect_res_paths(d[keys[i]], out);
            break;
        }
        case Variant::ARRAY: {
            const Array a = v;
            for (int i = 0; i < a.size(); ++i) collect_res_paths(a[i], out);
            break;
        }
        default: break;
    }
}

} // anonymous namespace

TestRunner::TestRunner(HandlerRegistry *registry)
    : PipelineRunnerBase(registry) {}

TestRunner::FileSnapshot TestRunner::take_snapshot() {
    FileSnapshot snap;
    EditorInterface *ei = EditorInterface::get_singleton();
    if (!ei) return snap;
    EditorFileSystem *efs = ei->get_resource_filesystem();
    if (!efs) return snap;
    efs->scan();
    EditorFileSystemDirectory *root = efs->get_filesystem();
    if (!root) return snap;
    std::deque<EditorFileSystemDirectory *> queue;
    queue.push_back(root);
    PackedStringArray all_paths;
    while (!queue.empty()) {
        EditorFileSystemDirectory *dir = queue[0];
        queue.pop_front();
        if (!dir) continue;
        for (int i = 0; i < dir->get_file_count(); ++i)
            all_paths.push_back(dir->get_file_path(i));
        for (int i = 0; i < dir->get_subdir_count(); ++i)
            queue.push_back(dir->get_subdir(i));
    }
    snap.paths = all_paths;
    return snap;
}

Array TestRunner::track_paths(const Dictionary &result) {
    Array paths;
    collect_res_paths(result, paths);
    return paths;
}

void TestRunner::cleanup(const Array &tracked,
                          const FileSnapshot &before,
                          const FileSnapshot &after,
                          Array &out_deleted,
                          Array &out_skipped) {
    HashMap<String, bool> before_set;
    for (int i = 0; i < before.paths.size(); ++i) before_set[before.paths[i]] = true;
    HashMap<String, bool> after_set;
    for (int i = 0; i < after.paths.size(); ++i) after_set[after.paths[i]] = true;
    std::vector<String> new_files;
    for (const auto &kv : after_set) {
        if (!before_set.has(kv.key)) new_files.push_back(kv.key);
    }
    HashMap<String, bool> tracked_set;
    for (int i = 0; i < tracked.size(); ++i) tracked_set[String(tracked[i])] = true;
    for (const String &path : new_files) {
        if (!tracked_set.has(path)) { out_skipped.push_back(path); continue; }
        if (path == "res://" || !path.begins_with("res://")) { out_skipped.push_back(path); continue; }
        Ref<DirAccess> da = DirAccess::open("res://");
        if (da.is_valid()) {
            const Error err = da->remove(path);
            if (err == OK) out_deleted.push_back(path);
            else { log_warn("test_engine", String("Failed to delete: ") + path); out_skipped.push_back(path); }
        }
        String dir_path = path.get_base_dir();
        for (int hop = 0; hop < 64 && dir_path != "res://" && dir_path.begins_with("res://"); ++hop) {
            Ref<DirAccess> d = DirAccess::open(dir_path);
            if (d.is_null()) break;
            bool empty = true;
            d->list_dir_begin();
            String n = d->get_next();
            while (!n.is_empty()) { if (n != "." && n != "..") { empty = false; break; } n = d->get_next(); }
            d->list_dir_end();
            if (empty) {
                Ref<DirAccess> parent = DirAccess::open(dir_path.get_base_dir());
                if (parent.is_valid()) {
                    const String dir_name = dir_path.get_file();
                    const Error rm_err = parent->remove(dir_name);
                    if (rm_err != OK) { log_warn("test_engine", String("Failed to remove empty dir: ") + dir_path); break; }
                } else break;
            } else break;
            dir_path = dir_path.get_base_dir();
        }
    }
}

void TestRunner::record_setting_before_call(const String &tool_name, const Dictionary &args) {
    if (tool_name != "set_setting" && tool_name != "reset_setting") return;
    const String path = args.get("setting_path", "");
    if (path.is_empty() || settings_snapshot_.has(path)) return;
    auto *ps = ProjectSettings::get_singleton();
    if (ps && ps->has_setting(path)) settings_snapshot_[path] = ps->get_setting(path);
    else settings_snapshot_[path] = Variant();
}

void TestRunner::restore_settings() {
    if (settings_snapshot_.size() == 0) return;
    auto *ps = ProjectSettings::get_singleton();
    if (!ps) return;
    const Array keys = settings_snapshot_.keys();
    for (int i = 0; i < keys.size(); ++i) {
        const String key = keys[i];
        const Variant val = settings_snapshot_[key];
        if (val.get_type() == Variant::NIL) { if (ps->has_setting(key)) ps->clear(key); }
        else ps->set_setting(key, val);
    }
    settings_snapshot_.clear();
}

void TestRunner::backup_project_godot() {
    Ref<FileAccess> fa = FileAccess::open("res://project.godot", FileAccess::READ);
    if (fa.is_null()) return;
    project_godot_backup_ = fa->get_as_text();
    fa->close();
}

void TestRunner::restore_project_godot() {
    if (project_godot_backup_.is_empty()) return;
    Ref<FileAccess> fa = FileAccess::open("res://project.godot", FileAccess::WRITE);
    if (fa.is_null()) { log_warn("test_engine", "Cannot write project.godot for restoration"); return; }
    fa->store_string(project_godot_backup_);
    fa->close();
    project_godot_backup_ = String();
    EditorInterface *ei = EditorInterface::get_singleton();
    if (ei) {
        EditorFileSystem *efs = ei->get_resource_filesystem();
        if (efs) efs->scan();
    }
}

Dictionary TestRunner::execute_step(const StepDef &step_def,
                                      PipelineContext &ctx,
                                      CallStats &stats) {
    Dictionary base_result = PipelineRunnerBase::execute_step(step_def, ctx, stats);
    const Dictionary raw_result = base_result.get("raw_result", Dictionary());
    const String assert_error = run_assertions(step_def.expect, raw_result);

    // TestRunner is authoritative for pass/fail — overrides base's pessimistic
    // decision when tool returned an expected error (e.g. undo with nothing to undo).
    String final_error;
    bool step_passed = false;
    String status_str = "FAIL";

    if (!assert_error.is_empty()) {
        if (step_def.allow_failure) {
            step_passed = true;
            status_str = "PASS";
        } else {
            final_error = assert_error;
        }
    } else if (!step_def.disk_verify.is_empty()) {
        const Array disk_errors = run_disk_verification(step_def.disk_verify);
        if (disk_errors.size() > 0) {
            const String disk_err = String("Disk verification failed: ") + JSON::stringify(disk_errors);
            if (step_def.allow_failure) {
                log_warn("test_engine", disk_err);
                step_passed = true;
                status_str = "PASS";
            } else {
                final_error = disk_err;
            }
        } else {
            step_passed = true;
            status_str = "PASS";
        }
    } else {
        // Old PipelineRunner behavior: no assertions + no disk_verify = always PASS.
        // Base's raw_result.has("error") check is ignored — TestRunner decides pass/fail,
        // not the raw tool response.
        step_passed = true;
        status_str = "PASS";
    }

    // Stats correction: base already counted tool error as call_fail,
    // but TestRunner may deem it PASS (e.g. expected error via expect block).
    // The old PipelineRunner tracked stats based on assertion outcome, so
    // we accept a minor discrepancy in info counters — the authoritative
    // pass/fail comes from step results, not stats counters.
    Dictionary result = base_result;
    result["passed"] = step_passed;
    result["status"] = status_str;
    if (!final_error.is_empty()) result["error"] = final_error;
    else result.erase("error");
    return result;
}

Dictionary TestRunner::run_test(const std::shared_ptr<PipelineDef> &pipeline) {
    Dictionary suite_result;
    suite_result["name"] = pipeline->name;
    suite_result["description"] = pipeline->description;
    const int64_t start_time_us = Time::get_singleton()->get_ticks_usec();
    // Clear UndoRedo history
    EditorInterface *ei = EditorInterface::get_singleton();
    if (ei) {
        EditorUndoRedoManager *ur = ei->get_editor_undo_redo();
        if (ur) ur->clear_history();
    }
    CallStats stats;
    backup_project_godot();
    const FileSnapshot before = take_snapshot();
    PipelineContext ctx;
    // before_all handled inside base run
    Array test_results;
    int total = 0, passed = 0, failed = 0, skipped = 0;
    // Run base execution
    RunResult run_result = PipelineRunnerBase::run(*pipeline, ctx);
    // Collect test results from RunResult
    for (const auto &sr : run_result.steps) {
        Dictionary entry;
        entry["tool"] = sr.tool;
        entry["step_id"] = sr.step_id;
        entry["passed"] = (sr.status == "PASS");
        entry["status"] = sr.status;
        if (!sr.error.is_empty()) entry["error"] = sr.error;
        entry["duration_ms"] = sr.duration_us / 1000;
        test_results.push_back(entry);
        if (sr.status == "PASS") passed++;
        else if (sr.status == "SKIP") skipped++;
        else failed++;
        total++;
    }
    restore_settings();
    restore_project_godot();
    const FileSnapshot after = take_snapshot();
    Array deleted, skipped_cleanup;
    cleanup(run_result.steps.is_empty() ? Array() : Array(), before, after, deleted, skipped_cleanup);
    const int64_t end_us = Time::get_singleton()->get_ticks_usec();
    Dictionary summary;
    summary["total"] = total;
    summary["passed"] = passed;
    summary["failed"] = failed;
    summary["skipped"] = skipped;
    summary["call_count"] = stats.call_count;
    summary["call_success"] = stats.call_success;
    summary["call_fail"] = stats.call_fail;
    summary["call_skip"] = stats.call_skip;
    {
        PackedStringArray ut;
        for (const auto &s : stats.unique_tools) ut.push_back(s);
        summary["unique_tools"] = ut;
    }
    {
        PackedStringArray us;
        for (const auto &s : stats.unique_success) us.push_back(s);
        summary["unique_success"] = us;
    }
    {
        PackedStringArray uf;
        for (const auto &s : stats.unique_fail) uf.push_back(s);
        summary["unique_fail"] = uf;
    }
    {
        PackedStringArray usk;
        for (const auto &s : stats.unique_skip) usk.push_back(s);
        summary["unique_skip"] = usk;
    }
    summary["duration_ms"] = (end_us - start_time_us) / 1000;
    summary["errors"] = stats.errors;
    summary["cleanup_deleted"] = deleted;
    summary["cleanup_skipped"] = skipped_cleanup;
    suite_result["summary"] = summary;
    suite_result["tests"] = test_results;
    return suite_result;
}

} // namespace pipeline
} // namespace godot_mcp
