#pragma once

#include "../testing/test_engine.hpp"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/editor_file_dialog.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/classes/tree.hpp>
#include <godot_cpp/classes/v_box_container.hpp>

namespace godot_mcp {

class TestRunnerDock : public godot::VBoxContainer {
    GDCLASS(TestRunnerDock, godot::VBoxContainer)

public:
    TestRunnerDock();

    void set_test_engine(TestEngine *engine) { engine_ = engine; }

protected:
    static void _bind_methods();

private:
    void _on_browse();
    void _on_file_selected(const godot::String &path);
    void _on_run();
    void _on_read();
    void _run_yaml(const godot::String &yaml_content);
    void populate_result_tree(const godot::Dictionary &result);

    godot::LineEdit *file_path_ = nullptr;
    godot::Button *browse_btn_ = nullptr;
    godot::Button *run_btn_ = nullptr;
    godot::Button *read_btn_ = nullptr;
    godot::Tree *result_tree_ = nullptr;
    godot::Label *status_label_ = nullptr;
    godot::EditorFileDialog *file_dialog_ = nullptr;
    TestEngine *engine_ = nullptr;
    godot::String current_yaml_;
};

} // namespace godot_mcp
