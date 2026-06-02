#include "test_runner_dock.hpp"
#include "../logging.hpp"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/theme.hpp>
#include <godot_cpp/classes/tree_item.hpp>

using namespace godot;

namespace godot_mcp {

TestRunnerDock::TestRunnerDock() {
    // ── Top bar: file path + buttons ──
    HBoxContainer *toolbar = memnew(HBoxContainer);
    toolbar->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    add_child(toolbar);

    file_path_ = memnew(LineEdit);
    file_path_->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    file_path_->set_placeholder("Select a .test.yaml file...");
    toolbar->add_child(file_path_);

    browse_btn_ = memnew(Button);
    browse_btn_->set_text("Browse");
    browse_btn_->connect("pressed", callable_mp(this, &TestRunnerDock::_on_browse));
    toolbar->add_child(browse_btn_);

    read_btn_ = memnew(Button);
    read_btn_->set_text("Read");
    read_btn_->connect("pressed", callable_mp(this, &TestRunnerDock::_on_read));
    toolbar->add_child(read_btn_);

    run_btn_ = memnew(Button);
    run_btn_->set_text("Run Tests");
    run_btn_->connect("pressed", callable_mp(this, &TestRunnerDock::_on_run));
    toolbar->add_child(run_btn_);

    // ── Status label ──
    status_label_ = memnew(Label);
    status_label_->set_text("Ready");
    status_label_->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    add_child(status_label_);

    // ── Result tree ──
    result_tree_ = memnew(Tree);
    result_tree_->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    result_tree_->set_v_size_flags(Control::SIZE_EXPAND_FILL);
    result_tree_->set_columns(2);
    result_tree_->set_column_titles_visible(true);
    result_tree_->set_column_title(0, "Test");
    result_tree_->set_column_title(1, "Result");
    result_tree_->set_hide_root(true);
    add_child(result_tree_);

    // ── File dialog ──
    file_dialog_ = memnew(EditorFileDialog);
    file_dialog_->set_file_mode(EditorFileDialog::FILE_MODE_OPEN_FILE);
    file_dialog_->add_filter("*.test.yaml;YAML Test Files");
    file_dialog_->add_filter("*.yaml;YAML Files");
    file_dialog_->add_filter("*.yml;YAML Files");
    file_dialog_->set_access(EditorFileDialog::ACCESS_RESOURCES);
    file_dialog_->connect("file_selected", callable_mp(this, &TestRunnerDock::_on_file_selected));
    add_child(file_dialog_);
}

void TestRunnerDock::_bind_methods() {}

void TestRunnerDock::_on_browse() {
    file_dialog_->popup_centered_ratio(0.5);
}

void TestRunnerDock::_on_file_selected(const String &path) {
    file_path_->set_text(path);
}

void TestRunnerDock::_on_read() {
    const String path = file_path_->get_text();
    if (path.is_empty()) {
        status_label_->set_text("No file selected");
        return;
    }

    Ref<FileAccess> f = FileAccess::open(path, FileAccess::READ);
    if (f.is_null()) {
        status_label_->set_text(String("Cannot open: ") + path);
        return;
    }

    current_yaml_ = f->get_as_text();
    f->close();

    status_label_->set_text(String("Loaded: ") + path + String(" (") +
                            String::num_int64(current_yaml_.utf8().length()) + String(" bytes)"));
}

void TestRunnerDock::_on_run() {
    if (current_yaml_.is_empty()) {
        status_label_->set_text("No YAML loaded. Click Read first.");
        return;
    }
    _run_yaml(current_yaml_);
}

void TestRunnerDock::_run_yaml(const String &yaml_content) {
    if (!engine_) {
        status_label_->set_text("Test engine not initialized");
        return;
    }

    status_label_->set_text("Running tests...");
    const Dictionary result = engine_->run(yaml_content);
    populate_result_tree(result);

    const Dictionary summary = result.get("summary", Dictionary());
    const int total = (int)summary.get("total", 0);
    const int passed = (int)summary.get("passed", 0);
    const int failed = (int)summary.get("failed", 0);
    const Array deleted = summary.get("cleanup_deleted", Array());
    const Array skipped = summary.get("cleanup_skipped", Array());

    String msg = String("Done: ") + String::num_int64(passed) + String("/") +
                 String::num_int64(total) + String(" passed");
    if (failed > 0) msg += String(", ") + String::num_int64(failed) + String(" failed");
    msg += String(" | Deleted ") + String::num_int64(deleted.size()) +
           String(" files, skipped ") + String::num_int64(skipped.size());
    status_label_->set_text(msg);
}

void TestRunnerDock::populate_result_tree(const Dictionary &result) {
    result_tree_->clear();

    TreeItem *root = result_tree_->create_item();

    // Suite info
    TreeItem *info = result_tree_->create_item(root);
    info->set_text(0, String("Suite: ") + String(result.get("name", "")));
    info->set_text(1, result.get("description", ""));

    // Summary
    const Dictionary summary = result.get("summary", Dictionary());
    TreeItem *sum_item = result_tree_->create_item(root);
    sum_item->set_text(0, "Summary");
    sum_item->set_text(1, String::num_int64((int)summary.get("passed", 0)) + String("/") +
                          String::num_int64((int)summary.get("total", 0)) + String(" passed"));

    // Test results
    const Array tests = result.get("tests", Array());
    for (int i = 0; i < tests.size(); ++i) {
        const Dictionary test = tests[i];
        TreeItem *test_item = result_tree_->create_item(root);
        const String tool = test.get("tool", "");
        const String desc = test.get("description", "");
        test_item->set_text(0, String::num_int64(i + 1) + String(". ") + tool + String(": ") + desc);
        const bool passed = test.get("passed", false);
        if (passed) {
            test_item->set_text(1, "PASS");
            test_item->set_custom_color(1, Color(0.2, 0.8, 0.2));
        } else {
            test_item->set_text(1, "FAIL");
            test_item->set_custom_color(1, Color(0.9, 0.2, 0.2));
            // Show error in child
            if (test.has("error")) {
                TreeItem *err_item = result_tree_->create_item(test_item);
                err_item->set_text(0, String("Error: ") + String(test["error"]));
                err_item->set_custom_color(0, Color(0.9, 0.2, 0.2));
            }
        }
    }

    // Cleanup info
    if (summary.has("cleanup_deleted")) {
        const Array deleted = summary["cleanup_deleted"];
        if (deleted.size() > 0) {
            TreeItem *cleanup = result_tree_->create_item(root);
            cleanup->set_text(0, String("Deleted ") + String::num_int64(deleted.size()) + String(" files"));
        }
    }
    if (summary.has("cleanup_skipped")) {
        const Array skipped = summary["cleanup_skipped"];
        if (skipped.size() > 0) {
            TreeItem *skipped_item = result_tree_->create_item(root);
            skipped_item->set_text(0, String("Skipped ") + String::num_int64(skipped.size()) + String(" files (user files)"));
            skipped_item->set_custom_color(0, Color(0.8, 0.8, 0.2));
        }
    }
}

} // namespace godot_mcp
