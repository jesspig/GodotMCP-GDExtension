#pragma once

#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/spin_box.hpp>
#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/classes/rich_text_label.hpp>
#include <godot_cpp/classes/h_separator.hpp>
#include <godot_cpp/classes/check_box.hpp>
#include <godot_cpp/classes/grid_container.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/scroll_container.hpp>

namespace godot_mcp {

class HandlerRegistry;
class McpEditorPlugin;
class McpLogger;

class McpDock : public godot::VBoxContainer {
    GDCLASS(McpDock, godot::VBoxContainer)

public:
    McpDock();
    ~McpDock() override;

    void set_plugin(McpEditorPlugin *p) { plugin_ = p; }
    void set_registry(HandlerRegistry *r) { registry_ = r; refresh_preview(); }
    void set_logger(McpLogger *l) { logger_ = l; }
    void update_status();

    void _process(double delta) override;

protected:
    static void _bind_methods();

private:
    // Status
    godot::Label *status_icon_ = nullptr;
    godot::Label *tools_count_ = nullptr;
    godot::Label *bridge_status_ = nullptr;

    // Client Config
    godot::OptionButton *client_selector_ = nullptr;
    godot::Button *generate_btn_ = nullptr;
    godot::Button *copy_btn_ = nullptr;
    godot::RichTextLabel *config_preview_ = nullptr;

    // Settings
    godot::SpinBox *http_port_spin_ = nullptr;
    godot::SpinBox *bridge_port_spin_ = nullptr;
    godot::OptionButton *bind_mode_ = nullptr;
    godot::LineEdit *custom_bind_addr_ = nullptr;
    godot::Button *apply_restart_btn_ = nullptr;
    godot::Button *force_restart_btn_ = nullptr;

    // Console Settings
    godot::SpinBox *max_entries_spin_ = nullptr;
    godot::LineEdit *log_dir_edit_ = nullptr;

    godot::String last_config_content_;

    McpEditorPlugin *plugin_ = nullptr;
    HandlerRegistry *registry_ = nullptr;
    McpLogger *logger_ = nullptr;

    double time_since_update_ = 0.0;

    // Callbacks
    void _on_generate_pressed();
    void _on_client_changed(int index);
    void _on_copy_pressed();
    void _on_apply_restart_pressed();
    void _on_force_restart_pressed();
    void _on_bind_mode_changed(int index);
    void _on_max_entries_changed(double value);
    void _on_log_dir_changed(const godot::String &text);

    void populate_client_list();
    godot::String get_selected_client() const;
    void refresh_preview();
};

} // namespace godot_mcp