#include "test_engine.hpp"
#include "../logging.hpp"

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/editor_file_system_directory.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/json.hpp>

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
// Execute a chain of tool calls (before_all, after_all, before_each, after_each)
// ---------------------------------------------------------------------------

godot::String TestEngine::execute_chain(const Array &chain, const char *chain_name) {
    for (int i = 0; i < chain.size(); ++i) {
        const Dictionary step = chain[i];
        const String tool_name = step.get("tool", "");
        const Dictionary tool_args = step.get("args", Dictionary());
        if (tool_name.is_empty()) continue;
        const Dictionary result = registry_->execute(tool_name, tool_args);
        if (result.has("error")) {
            const Variant err = result["error"];
            String err_msg = err.get_type() == Variant::DICTIONARY
                ? String(Dictionary(err).get("message", "Unknown error"))
                : String(err);
            return String(chain_name) + String(" step '") + tool_name + String("' failed: ") + err_msg;
        }
        if (result.has("success") && !result["success"].operator bool()) {
            return String(chain_name) + String(" step '") + tool_name + String("' returned success=false");
        }
    }
    return String();
}

// ---------------------------------------------------------------------------
// Execute a single test and return its result Dictionary
// ---------------------------------------------------------------------------

Dictionary TestEngine::execute_test(const Dictionary &test_def) {
    Dictionary test_result;
    const String tool_name = test_def.get("tool", "");
    const String description = test_def.get("description", "");
    const Dictionary tool_args = test_def.get("args", Dictionary());
    const Dictionary expect = test_def.get("expect", Dictionary());
    const Dictionary disk_verify = test_def.get("disk_verify", Dictionary());

    test_result["tool"] = tool_name;
    test_result["description"] = description;

    // Execute tool
    const Dictionary raw_result = registry_->execute(tool_name, tool_args);
    test_result["raw_result"] = raw_result;
    test_result["tracked_paths"] = track_paths(raw_result);

    // Run assertions
    const String assert_error = run_assertions(expect, raw_result);
    if (!assert_error.is_empty()) {
        test_result["passed"] = false;
        test_result["error"] = assert_error;
        return test_result;
    }

    // Run disk verification
    if (!disk_verify.is_empty()) {
        const Array disk_errors = run_disk_verification(disk_verify);
        if (disk_errors.size() > 0) {
            test_result["passed"] = false;
            test_result["error"] = String("Disk verification failed: ") + JSON::stringify(disk_errors);
            return test_result;
        }
    }

    test_result["passed"] = true;
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

    // --- Snapshot before ---
    const FileSnapshot before = take_snapshot();

    // --- before_all chain ---
    String before_all_error;
    if (config.has("before_all")) {
        before_all_error = execute_chain(config["before_all"], "before_all");
    }

    // --- Tests ---
    Array test_results;
    int total = 0, passed = 0, failed = 0;
    bool teardown_failed = false;
    String teardown_error;

    if (!before_all_error.is_empty()) {
        // before_all failed — report error and skip all tests
        if (config.has("tests")) {
            const Array tests = config["tests"];
            total = static_cast<int>(tests.size());
            failed = total;
            for (int i = 0; i < tests.size(); ++i) {
                const Dictionary test_def = tests[i];
                Dictionary entry;
                entry["tool"] = test_def.get("tool", "");
                entry["description"] = test_def.get("description", "");
                entry["passed"] = false;
                entry["error"] = before_all_error;
                test_results.push_back(entry);
            }
        }
    } else if (config.has("tests")) {
        const Array tests = config["tests"];
        total = static_cast<int>(tests.size());

        for (int i = 0; i < tests.size(); ++i) {
            const Dictionary test_def = tests[i];

            // before_each
            String before_each_err;
            if (config.has("before_each")) {
                before_each_err = execute_chain(config["before_each"], "before_each");
            }

            Dictionary result;
            if (!before_each_err.is_empty()) {
                result["tool"] = test_def.get("tool", "");
                result["description"] = test_def.get("description", "");
                result["passed"] = false;
                result["error"] = before_each_err;
            } else {
                // Execute the test
                result = execute_test(test_def);

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
                const String after_each_err = execute_chain(config["after_each"], "after_each");
                if (!after_each_err.is_empty()) {
                    log_warn("test_engine", after_each_err);
                    // A teardown failure leaves the suite in a dirty state; the
                    // old code only log_warn'd and left the test green, which
                    // hid real problems. Flip the result to failed and surface
                    // the teardown error.
                    result["passed"] = false;
                    result["error"] = String("after_each failed: ") + after_each_err;
                }
            }

            // Gather result (strip internal fields)
            Dictionary entry;
            entry["tool"] = result.get("tool", "");
            entry["description"] = result.get("description", "");
            entry["passed"] = result.get("passed", false);
            if (result.has("error")) {
                entry["error"] = result["error"];
                failed++;
            } else {
                passed++;
            }
            test_results.push_back(entry);
        }
    }

    // --- after_all chain ---
    if (config.has("after_all")) {
        const String after_all_err = execute_chain(config["after_all"], "after_all");
        if (!after_all_err.is_empty()) {
            log_warn("test_engine", after_all_err);
            // Surface suite-level teardown failure instead of swallowing it.
            teardown_failed = true;
            teardown_error = after_all_err;
        }
    }

    // --- Snapshot after + cleanup ---
    const FileSnapshot after = take_snapshot();

    Array all_tracked = suite_result.get("_all_tracked", Array());
    Array deleted, skipped;
    cleanup(all_tracked, before, after, deleted, skipped);

    // --- Build summary ---
    Dictionary summary;
    summary["total"] = total;
    summary["passed"] = passed;
    summary["failed"] = failed;
    summary["cleanup_deleted"] = deleted;
    summary["cleanup_skipped"] = skipped;
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
