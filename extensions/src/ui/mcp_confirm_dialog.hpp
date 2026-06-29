#pragma once

#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/rich_text_label.hpp>
#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/classes/h_separator.hpp>

namespace godot_mcp {

class McpConfirmDialog : public godot::Window {
    GDCLASS(McpConfirmDialog, godot::Window)

public:
    McpConfirmDialog();
    ~McpConfirmDialog() override;

    void show_for_op(const godot::Variant &id, const godot::String &tool_name,
                     const godot::Dictionary &args);

protected:
    static void _bind_methods();

private:
    godot::Label *tool_name_label_ = nullptr;
    godot::RichTextLabel *params_view_ = nullptr;
    godot::Button *deny_btn_ = nullptr;
    godot::Button *allow_btn_ = nullptr;
    godot::Button *allow_all_btn_ = nullptr;

    godot::Variant current_id_;

    void _on_deny_pressed();
    void _on_allow_pressed();
    void _on_allow_all_pressed();
    void _on_close_requested();
};

} // namespace godot_mcp
