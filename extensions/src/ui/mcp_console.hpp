#pragma once

#include "mcp_logger.hpp"

#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/classes/tree.hpp>
#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/check_box.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/v_separator.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/popup_menu.hpp>
#include <godot_cpp/classes/text_edit.hpp>
#include <godot_cpp/classes/code_edit.hpp>
#include <godot_cpp/classes/v_split_container.hpp>
#include <godot_cpp/classes/h_split_container.hpp>

namespace godot_mcp {

class McpConsole : public godot::VBoxContainer {
    GDCLASS(McpConsole, godot::VBoxContainer)

public:
    McpConsole();
    ~McpConsole() override;

    void set_logger(McpLogger *logger);
    void on_log_appended(const McpLogger::LogEntry &entry);
    void refresh();

protected:
    static void _bind_methods();

private:
    godot::Tree *log_tree_ = nullptr;

    godot::Button *expand_btn_ = nullptr;
    godot::Button *collapse_btn_ = nullptr;
    godot::Button *clear_btn_ = nullptr;
    godot::CheckBox *auto_scroll_chk_ = nullptr;
    godot::Label *count_label_ = nullptr;

    godot::CodeEdit *req_view_ = nullptr;
    godot::CodeEdit *res_view_ = nullptr;

    godot::PopupMenu *context_menu_ = nullptr;

    godot::Label *time_header_ = nullptr;
    godot::Control *drag_area_ = nullptr;

    McpLogger *logger_ = nullptr;
    bool auto_scroll_ = true;
    static constexpr int kMaxVisible = 500;

    godot::Vector<McpLogger::LogEntry> logged_entries_;

    void _on_clear_pressed();
    void _on_expand_all();
    void _on_collapse_all();
    void _on_auto_scroll_toggled(bool pressed);
    void _on_item_activated();
    void _on_item_selected();
    void _on_item_mouse_selected(const godot::Vector2 &pos, int32_t button);
    void _on_context_menu_id_pressed(int32_t id);
    void _on_drag_area_gui_input(const godot::Ref<godot::InputEvent> &event);

    void setup_tree_columns();
    void rebuild_log();
    void add_tree_entry(const McpLogger::LogEntry &entry, int index);
    void update_detail();
    void update_toolbar_state();
    void rebuild_metadata_indices();
    int visible_count() const;

    static godot::String fmt_tool(const McpLogger::LogEntry &e);
    static godot::String json_compact(const godot::Variant &v);
    static godot::String format_json(const godot::Variant &v);
    static godot::String format_entry(const McpLogger::LogEntry &entry, bool with_children);
};

} // namespace godot_mcp
