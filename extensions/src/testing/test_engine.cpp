#include "test_engine.hpp"
#include "../logging.hpp"

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/editor_file_system_directory.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/classes/project_settings.hpp>

#include <deque>

using namespace godot;

namespace godot_mcp {

// ---------------------------------------------------------------------------
// FileSystem snapshot — walk the in-memory EditorFileSystem tree
// ---------------------------------------------------------------------------

TestEngine::FileSnapshot TestEngine::take_snapshot() {
    FileSnapshot snap;
    EditorInterface *ei = EditorInterface::get_singleton();
    if (!ei) return snap;

    EditorFileSystem *efs = ei->get_resource_filesystem();
    if (!efs) return snap;

    // Force a rescan so the in-memory cache reflects the actual disk state.
    // Without this, a tool that just wrote a file may not appear in `after`,
    // so cleanup() would miss it (or worse, a stale cache entry could cause a
    // pre-existing file to be misclassified as "new" and deleted.
    efs->scan();

    EditorFileSystemDirectory *root = efs->get_filesystem();
    if (!root) return snap;

    // BFS over the file tree
    std::deque<EditorFileSystemDirectory *> queue;
    queue.push_back(root);

    PackedStringArray all_paths;
    while (!queue.empty()) {
        EditorFileSystemDirectory *dir = queue[0];
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

// ---------------------------------------------------------------------------
// Track res:// paths from tool result (recursive search)
// ---------------------------------------------------------------------------

namespace {

void collect_res_paths(const Variant &v, Array &out) {
    switch (v.get_type()) {
        case Variant::STRING: {
            const String s = v;
            if (s.begins_with("res://")) {
                out.push_back(s);
            }
            break;
        }
        case Variant::DICTIONARY: {
            const Dictionary d = v;
            const Array keys = d.keys();
            for (int i = 0; i < keys.size(); ++i) {
                collect_res_paths(d[keys[i]], out);
            }
            break;
        }
        case Variant::ARRAY: {
            const Array a = v;
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

Array TestEngine::track_paths(const Dictionary &result) {
    Array paths;
    collect_res_paths(result, paths);
    return paths;
}

// ---------------------------------------------------------------------------
// Cleanup: delete files that are in both (after - before) AND tracked
// ---------------------------------------------------------------------------

void TestEngine::cleanup(const Array &tracked,
                          const FileSnapshot &before,
                          const FileSnapshot &after,
                          Array &out_deleted,
                          Array &out_skipped) {
    // Build set of before paths
    HashMap<String, bool> before_set;
    for (int i = 0; i < before.paths.size(); ++i) {
        before_set[before.paths[i]] = true;
    }

    // Build set of after paths
    HashMap<String, bool> after_set;
    for (int i = 0; i < after.paths.size(); ++i) {
        after_set[after.paths[i]] = true;
    }

    // Find new files (after - before)
    Vector<String> new_files;
    for (const auto &kv : after_set) {
        if (!before_set.has(kv.key)) {
            new_files.push_back(kv.key);
        }
    }

    // Build set of tracked paths
    HashMap<String, bool> tracked_set;
    for (int i = 0; i < tracked.size(); ++i) {
        tracked_set[String(tracked[i])] = true;
    }

    // Only delete files in both (intersection)
    for (const String &path : new_files) {
        if (!tracked_set.has(path)) {
            out_skipped.push_back(path);
            continue;
        }

        // Safety: never touch anything outside res:// or res:// itself.
        // The diff comes from the EditorFileSystem cache, which should only
        // ever contain res:// paths, but guard defensively against a
        // malformed/symlinked layout escaping the project root.
        if (path == "res://" || !path.begins_with("res://")) {
            out_skipped.push_back(path);
            continue;
        }

        // Delete file
        Ref<DirAccess> da = DirAccess::open("res://");
        if (da.is_valid()) {
            const Error err = da->remove(path);
            if (err == OK) {
                out_deleted.push_back(path);
            } else {
                log_warn("test_engine", String("Failed to delete: ") + path);
                out_skipped.push_back(path);
            }
        }

        // Clean up empty parent directories (walk up, stop at res://).
        // Cap iterations to defend against pathological/symlink layouts that
        // could otherwise make get_base_dir() spin, and check every remove()'s
        // return value (the old code ignored failures silently).
        String dir_path = path.get_base_dir();
        for (int hop = 0; hop < 64 && dir_path != "res://" && dir_path.begins_with("res://"); ++hop) {
            Ref<DirAccess> d = DirAccess::open(dir_path);
            if (d.is_null()) break;

            // Check if directory is empty
            bool empty = true;
            d->list_dir_begin();
            String n = d->get_next();
            while (!n.is_empty()) {
                if (n != "." && n != "..") { empty = false; break; }
                n = d->get_next();
            }
            d->list_dir_end();

            if (empty) {
                // Remove this empty directory
                Ref<DirAccess> parent = DirAccess::open(dir_path.get_base_dir());
                if (parent.is_valid()) {
                    const String dir_name = dir_path.get_file();
                    const Error rm_err = parent->remove(dir_name);
                    if (rm_err != OK) {
                        log_warn("test_engine",
                                 String("Failed to remove empty dir: ") + dir_path);
                        break;  // stop walking if we can't remove this level
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

// ---------------------------------------------------------------------------
// Settings snapshot — record initial values before mutation
// ---------------------------------------------------------------------------

void TestEngine::record_setting_before_call(const String &tool_name,
                                             const Dictionary &args) {
    if (tool_name != "set_setting" && tool_name != "reset_setting")
        return;
    const String path = args.get("setting_path", "");
    if (path.is_empty() || settings_snapshot_.has(path))
        return;

    auto *ps = ProjectSettings::get_singleton();
    if (ps && ps->has_setting(path)) {
        settings_snapshot_[path] = ps->get_setting(path);
    } else {
        settings_snapshot_[path] = Variant(); // NIL = didn't exist
    }
}

void TestEngine::restore_settings() {
    if (settings_snapshot_.size() == 0)
        return;
    auto *ps = ProjectSettings::get_singleton();
    if (!ps)
        return;

    const Array keys = settings_snapshot_.keys();
    for (int i = 0; i < keys.size(); ++i) {
        const String key = keys[i];
        const Variant val = settings_snapshot_[key];
        if (val.get_type() == Variant::NIL) {
            if (ps->has_setting(key))
                ps->clear(key);
        } else {
            ps->set_setting(key, val);
        }
    }
    // NOTE: intentionally no ps->save() here — the file on disk was already
    // rewritten by the tool's ps->save() calls with Godot's internal
    // formatting. We restore in-memory values only; the raw file content is
    // restored separately in restore_project_godot() to preserve the original
    // formatting, comments, and section ordering.
    settings_snapshot_.clear();
}

void TestEngine::backup_project_godot() {
    Ref<FileAccess> fa = FileAccess::open("res://project.godot", FileAccess::READ);
    if (fa.is_null()) return;
    project_godot_backup_ = fa->get_as_text();
    fa->close();
}

void TestEngine::restore_project_godot() {
    if (project_godot_backup_.is_empty()) return;

    Ref<FileAccess> fa = FileAccess::open("res://project.godot", FileAccess::WRITE);
    if (fa.is_null()) {
        log_warn("test_engine", "Cannot write project.godot for restoration");
        return;
    }
    fa->store_string(project_godot_backup_);
    fa->close();
    project_godot_backup_ = String();

    // Refresh the editor file system cache so the restored file is visible.
    EditorInterface *ei = EditorInterface::get_singleton();
    if (ei) {
        EditorFileSystem *efs = ei->get_resource_filesystem();
        if (efs) efs->scan();
    }
}

// ---------------------------------------------------------------------------
// Execute a chain of tool calls (before_all, after_all, before_each, after_each)
// Returns {error, skipped_count, errors}
// ---------------------------------------------------------------------------

Dictionary TestEngine::execute_chain(const Array &chain, const char *chain_name,
                                      const String &on_failure, CallStats &stats) {
    Dictionary result;
    result["error"] = String();
    result["skipped_count"] = 0;
    result["errors"] = Array();

    Array chain_errors;
    const int chain_size = static_cast<int>(chain.size());

    for (int i = 0; i < chain_size; ++i) {
        const Dictionary step = chain[i];
        const String tool_name = step.get("tool", "");
        const Dictionary tool_args = step.get("args", Dictionary());
        if (tool_name.is_empty()) continue;

        // Per-step on_failure overrides the chain-level default
        const String step_on_failure = step.has("on_failure")
            ? String(step["on_failure"])
            : on_failure;

        record_setting_before_call(tool_name, tool_args);
        const Dictionary exec_result = registry_->execute(tool_name, tool_args);

        stats.call_count++;
        stats.unique_tools.insert(tool_name);

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
            stats.unique_fail.insert(tool_name);

            const String full_err = String(chain_name) + String(" step '") + tool_name + String("' failed: ") + err_msg;

            Dictionary err_entry;
            err_entry["tool"] = tool_name;
            err_entry["error"] = full_err;
            chain_errors.push_back(err_entry);
            stats.errors.push_back(err_entry);

            // Record skipped steps for skip_remaining
            if (step_on_failure == "skip_remaining") {
                const int remaining = chain_size - i - 1;
                stats.call_skip += remaining;
                for (int s = i + 1; s < chain_size; ++s) {
                    const Dictionary skipped_step = chain[s];
                    const String skipped_tool = skipped_step.get("tool", "");
                    if (!skipped_tool.is_empty()) {
                        stats.unique_skip.insert(skipped_tool);
                    }
                }
                result["error"] = full_err;
                result["skipped_count"] = remaining;
                result["errors"] = chain_errors;
                return result;
            } else if (step_on_failure == "stop") {
                result["error"] = full_err;
                result["errors"] = chain_errors;
                return result;
            }
            // "continue": record error, keep going
        } else {
            stats.call_success++;
            stats.unique_success.insert(tool_name);
        }
    }

    result["errors"] = chain_errors;
    return result;
}

// ---------------------------------------------------------------------------
// Execute a single test and return its result Dictionary
// ---------------------------------------------------------------------------

Dictionary TestEngine::execute_test(const Dictionary &test_def, CallStats &stats) {
    Dictionary test_result;
    const String tool_name = test_def.get("tool", "");
    const String description = test_def.get("description", "");
    const Dictionary tool_args = test_def.get("args", Dictionary());
    const Dictionary expect = test_def.get("expect", Dictionary());
    const Dictionary disk_verify = test_def.get("disk_verify", Dictionary());

    test_result["tool"] = tool_name;
    test_result["description"] = description;

    const int64_t start_us = Time::get_singleton()->get_ticks_usec();

    // Execute tool
    record_setting_before_call(tool_name, tool_args);
    const Dictionary raw_result = registry_->execute(tool_name, tool_args);

    stats.call_count++;
    stats.unique_tools.insert(tool_name);

    test_result["raw_result"] = raw_result;
    test_result["tracked_paths"] = track_paths(raw_result);

    // Run assertions
    const String assert_error = run_assertions(expect, raw_result);
    if (!assert_error.is_empty()) {
        stats.call_fail++;
        stats.unique_fail.insert(tool_name);

        Dictionary err_entry;
        err_entry["tool"] = tool_name;
        err_entry["error"] = assert_error;
        stats.errors.push_back(err_entry);

        test_result["passed"] = false;
        test_result["status"] = "FAIL";
        test_result["error"] = assert_error;
        test_result["duration_us"] = Time::get_singleton()->get_ticks_usec() - start_us;
        return test_result;
    }

    // Run disk verification
    if (!disk_verify.is_empty()) {
        const Array disk_errors = run_disk_verification(disk_verify);
        if (disk_errors.size() > 0) {
            stats.call_fail++;
            stats.unique_fail.insert(tool_name);

            const String disk_err = String("Disk verification failed: ") + JSON::stringify(disk_errors);
            Dictionary err_entry;
            err_entry["tool"] = tool_name;
            err_entry["error"] = disk_err;
            stats.errors.push_back(err_entry);

            test_result["passed"] = false;
            test_result["status"] = "FAIL";
            test_result["error"] = disk_err;
            test_result["duration_us"] = Time::get_singleton()->get_ticks_usec() - start_us;
            return test_result;
        }
    }

    stats.call_success++;
    stats.unique_success.insert(tool_name);

    test_result["passed"] = true;
    test_result["status"] = "PASS";
    test_result["duration_us"] = Time::get_singleton()->get_ticks_usec() - start_us;
    return test_result;
}

// ---------------------------------------------------------------------------
// Main entry: run a complete test suite from YAML
// ---------------------------------------------------------------------------

Dictionary TestEngine::run(const String &yaml_content) {
    Dictionary suite_result;

    // Parse YAML
    const Variant parsed = parse_yaml(yaml_content);
    if (parsed.get_type() != Variant::DICTIONARY) {
        Dictionary err;
        err["error"] = String("YAML top-level must be a mapping (dictionary)");
        return err;
    }
    const Dictionary config = parsed;

    suite_result["name"] = config.get("name", "");
    suite_result["description"] = config.get("description", "");

    const int64_t start_time_us = Time::get_singleton()->get_ticks_usec();

    CallStats stats;

    const String suite_on_failure = config.get("on_failure", "stop");

    // --- Backup project.godot raw content (before any tool touches it) ---
    backup_project_godot();

    // --- Snapshot before ---
    const FileSnapshot before = take_snapshot();

    // --- before_all chain ---
    Dictionary before_all_result;
    bool before_all_failed = false;
    if (config.has("before_all")) {
        before_all_result = execute_chain(config["before_all"], "before_all", suite_on_failure, stats);
        if (!String(before_all_result["error"]).is_empty()) {
            before_all_failed = true;
        }
    }

    // --- Tests ---
    Array test_results;
    int total = 0, passed = 0, failed = 0, skipped = 0;
    bool teardown_failed = false;
    String teardown_error;
    bool skip_remaining = false;

    if (before_all_failed) {
        // before_all failed — skip all tests
        if (config.has("tests")) {
            const Array tests = config["tests"];
            total = static_cast<int>(tests.size());
            skipped = total;
            stats.call_skip += total;
            for (int i = 0; i < tests.size(); ++i) {
                const Dictionary test_def = tests[i];
                Dictionary entry;
                entry["tool"] = test_def.get("tool", "");
                entry["description"] = test_def.get("description", "");
                entry["passed"] = false;
                entry["status"] = "SKIP";
                entry["error"] = before_all_result["error"];
                test_results.push_back(entry);
            }
        }
    } else if (config.has("tests")) {
        const Array tests = config["tests"];
        total = static_cast<int>(tests.size());

        for (int i = 0; i < tests.size(); ++i) {
            const Dictionary test_def = tests[i];

            if (skip_remaining) {
                Dictionary entry;
                entry["tool"] = test_def.get("tool", "");
                entry["description"] = test_def.get("description", "");
                entry["passed"] = false;
                entry["status"] = "SKIP";
                skipped++;
                stats.call_skip++;
                test_results.push_back(entry);
                continue;
            }

            const String test_on_failure = test_def.has("on_failure")
                ? String(test_def["on_failure"])
                : suite_on_failure;

            // before_each
            Dictionary before_each_result;
            bool before_each_failed = false;
            if (config.has("before_each")) {
                before_each_result = execute_chain(config["before_each"], "before_each", test_on_failure, stats);
                if (!String(before_each_result["error"]).is_empty()) {
                    before_each_failed = true;
                }
            }

            Dictionary result;
            if (before_each_failed) {
                result["tool"] = test_def.get("tool", "");
                result["description"] = test_def.get("description", "");
                result["passed"] = false;
                result["status"] = "FAIL";
                result["error"] = before_each_result["error"];
                result["duration_us"] = 0;
            } else {
                // Execute the test
                result = execute_test(test_def, stats);

                // Collect tracked paths
                if (result.has("tracked_paths")) {
                    const Array paths = result["tracked_paths"];
                    if (!suite_result.has("_all_tracked")) {
                        suite_result["_all_tracked"] = Array();
                    }
                    Array all_tracked = suite_result["_all_tracked"];
                    const int old_size = static_cast<int>(all_tracked.size());
                    all_tracked.resize(old_size + paths.size());
                    for (int p = 0; p < paths.size(); ++p) {
                        all_tracked[old_size + p] = paths[p];
                    }
                    suite_result["_all_tracked"] = all_tracked;
                }
            }

            // after_each
            if (config.has("after_each")) {
                const Dictionary after_each_result = execute_chain(config["after_each"], "after_each", test_on_failure, stats);
                if (!String(after_each_result["error"]).is_empty()) {
                    log_warn("test_engine", String(after_each_result["error"]));
                    result["passed"] = false;
                    result["status"] = "FAIL";
                    result["error"] = String("after_each failed: ") + String(after_each_result["error"]);
                }
            }

            // Gather result (strip internal fields)
            Dictionary entry;
            entry["tool"] = result.get("tool", "");
            entry["description"] = result.get("description", "");
            entry["passed"] = result.get("passed", false);
            entry["status"] = result.get("status", "PASS");
            if (result.has("duration_us")) {
                entry["duration_ms"] = static_cast<int64_t>(result["duration_us"]) / 1000;
            }
            if (result.has("error")) {
                entry["error"] = result["error"];
            }

            const String status = result.get("status", "");
            if (status == "PASS") {
                passed++;
            } else if (status == "SKIP") {
                skipped++;
            } else {
                failed++;
            }

            // Handle on_failure for test-level skip_remaining
            if (status == "FAIL" && test_on_failure == "skip_remaining") {
                skip_remaining = true;
            }

            test_results.push_back(entry);
        }
    }

    // --- after_all chain ---
    if (config.has("after_all")) {
        const Dictionary after_all_result = execute_chain(config["after_all"], "after_all", suite_on_failure, stats);
        if (!String(after_all_result["error"]).is_empty()) {
            log_warn("test_engine", String(after_all_result["error"]));
            teardown_failed = true;
            teardown_error = after_all_result["error"];
        }
    }

    // --- Restore project settings to pre-test values ---
    restore_settings();       // in-memory: set back pre-test values
    restore_project_godot();  // on-disk: byte-identical backup

    // --- Snapshot after + cleanup ---
    const FileSnapshot after = take_snapshot();

    Array all_tracked = suite_result.get("_all_tracked", Array());
    Array deleted, skipped_cleanup;
    cleanup(all_tracked, before, after, deleted, skipped_cleanup);

    const int64_t end_time_us = Time::get_singleton()->get_ticks_usec();
    const int64_t duration_ms = (end_time_us - start_time_us) / 1000;

    // --- Build summary ---
    Dictionary summary;
    summary["total"] = total;
    summary["passed"] = passed;
    summary["failed"] = failed;
    summary["skipped"] = skipped;
    summary["call_count"] = stats.call_count;
    summary["call_success"] = stats.call_success;
    summary["call_fail"] = stats.call_fail;
    summary["call_skip"] = stats.call_skip;

    // Convert std::set<String> to PackedStringArray
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

    summary["duration_ms"] = duration_ms;
    summary["errors"] = stats.errors;
    summary["cleanup_deleted"] = deleted;
    summary["cleanup_skipped"] = skipped_cleanup;
    if (teardown_failed) {
        summary["teardown_failed"] = true;
        summary["teardown_error"] = teardown_error;
    }

    suite_result["summary"] = summary;
    suite_result["tests"] = test_results;

    // Remove internal fields
    if (suite_result.has("_all_tracked")) {
        suite_result.erase("_all_tracked");
    }

    return suite_result;
}

} // namespace godot_mcp
