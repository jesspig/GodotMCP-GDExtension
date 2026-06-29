#include "mcp_confirm_dialog.hpp"
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/classes/window.hpp>

using namespace godot;

namespace godot_mcp {

McpConfirmDialog::McpConfirmDialog() {
    set_title("Destructive Operation - Confirm");
    set_size(Vector2(480, 320));
    set_exclusive(true);
    set_min_size(Vector2(360, 240));
    set_flag(Window::FLAG_ALWAYS_ON_TOP, true);

    VBoxContainer *vbox = memnew(VBoxContainer);
    vbox->set_anchors_preset(Control::PRESET_FULL_RECT);
    vbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    vbox->set_v_size_flags(Control::SIZE_EXPAND_FILL);
    add_child(vbox);

    Label *header = memnew(Label);
    header->set_text("Warning: This operation cannot be undone.");
    header->add_theme_color_override("font_color", Color(1.0f, 0.8f, 0.2f));
    vbox->add_child(header);

    Label *tool_label_header = memnew(Label);
    tool_label_header->set_text("Tool:");
    vbox->add_child(tool_label_header);

    tool_name_label_ = memnew(Label);
    tool_name_label_->add_theme_font_size_override("font_size", 18);
    tool_name_label_->add_theme_color_override("font_color", Color(1.0f, 0.4f, 0.4f));
    vbox->add_child(tool_name_label_);

    Label *params_header = memnew(Label);
    params_header->set_text("Parameters:");
    vbox->add_child(params_header);

    params_view_ = memnew(RichTextLabel);
    params_view_->set_custom_minimum_size(Vector2(0, 120));
    params_view_->set_v_size_flags(Control::SIZE_EXPAND_FILL);
    params_view_->set_selection_enabled(true);
    params_view_->set_use_bbcode(false);
    vbox->add_child(params_view_);

    vbox->add_child(memnew(HSeparator));

    HBoxContainer *btn_row = memnew(HBoxContainer);
    vbox->add_child(btn_row);

    deny_btn_ = memnew(Button);
    deny_btn_->set_text("Deny");
    deny_btn_->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    btn_row->add_child(deny_btn_);

    allow_btn_ = memnew(Button);
    allow_btn_->set_text("Allow");
    allow_btn_->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    btn_row->add_child(allow_btn_);

    allow_all_btn_ = memnew(Button);
    allow_all_btn_->set_text("Allow All This Session");
    allow_all_btn_->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    btn_row->add_child(allow_all_btn_);

    deny_btn_->connect("pressed", Callable(this, "_on_deny_pressed"));
    allow_btn_->connect("pressed", Callable(this, "_on_allow_pressed"));
    allow_all_btn_->connect("pressed", Callable(this, "_on_allow_all_pressed"));

    if (is_connected("close_requested", Callable(this, "_on_close_requested"))) {
        disconnect("close_requested", Callable(this, "_on_close_requested"));
    }
    connect("close_requested", Callable(this, "_on_close_requested"));
}

McpConfirmDialog::~McpConfirmDialog() = default;

void McpConfirmDialog::_bind_methods() {
    ClassDB::bind_method(D_METHOD("_on_deny_pressed"), &McpConfirmDialog::_on_deny_pressed);
    ClassDB::bind_method(D_METHOD("_on_allow_pressed"), &McpConfirmDialog::_on_allow_pressed);
    ClassDB::bind_method(D_METHOD("_on_allow_all_pressed"), &McpConfirmDialog::_on_allow_all_pressed);
    ClassDB::bind_method(D_METHOD("_on_close_requested"), &McpConfirmDialog::_on_close_requested);

    ADD_SIGNAL(MethodInfo("confirmed", PropertyInfo(Variant::NIL, "id")));
    ADD_SIGNAL(MethodInfo("denied", PropertyInfo(Variant::NIL, "id")));
    ADD_SIGNAL(MethodInfo("allow_all_session", PropertyInfo(Variant::NIL, "id")));
}

void McpConfirmDialog::show_for_op(const Variant &id, const String &tool_name,
                                    const Dictionary &args) {
    current_id_ = id;
    tool_name_label_->set_text(tool_name);
    params_view_->set_text(JSON::stringify(args, "  "));

    set_size(Vector2(480, 320));
    Node *parent = get_parent();
    if (parent) {
        parent->remove_child(this);
    }
    EditorInterface::get_singleton()->popup_dialog_centered(this, Vector2(480, 320));
}

void McpConfirmDialog::_on_deny_pressed() {
    hide();
    emit_signal("denied", current_id_);
}

void McpConfirmDialog::_on_allow_pressed() {
    hide();
    emit_signal("confirmed", current_id_);
}

void McpConfirmDialog::_on_allow_all_pressed() {
    hide();
    emit_signal("allow_all_session", current_id_);
}

void McpConfirmDialog::_on_close_requested() {
    hide();
    emit_signal("denied", current_id_);
}

} // namespace godot_mcp
