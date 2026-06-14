// =====================================================================
// mcp_dock.cpp — GodotMCP 右侧边栏 Dock
// =====================================================================

#include "mcp_dock.hpp"
#include "client_registry.hpp"
#include "editor_plugin.hpp"
#include "server/registry/handler_registry.hpp"
#include "mcp_logger.hpp"

#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/project_settings.hpp>

using namespace godot;

namespace godot_mcp {

// ---------------------------------------------------------------------
// 构造 / 析构
// ---------------------------------------------------------------------

McpDock::McpDock() {
    set_name("Godot MCP");
    set_custom_minimum_size(Vector2(320, 0));

    // ScrollContainer for the entire dock content
    ScrollContainer *scroll = memnew(ScrollContainer);
    scroll->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    scroll->set_v_size_flags(Control::SIZE_EXPAND_FILL);
    scroll->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
    add_child(scroll);

    VBoxContainer *content = memnew(VBoxContainer);
    content->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    scroll->add_child(content);

    // ── Status Section ──
    Label *status_label = memnew(Label);
    status_label->set_text("Status");
    status_label->add_theme_font_size_override("font_size", 16);
    content->add_child(status_label);

    HBoxContainer *status_row = memnew(HBoxContainer);
    content->add_child(status_row);

    status_icon_ = memnew(Label);
    status_icon_->set_text("[ON]");
    status_icon_->add_theme_color_override("font_color", Color(0.2f, 0.9f, 0.2f));
    status_row->add_child(status_icon_);

    tools_count_ = memnew(Label);
    tools_count_->set_text("Tools: 0");
    status_row->add_child(tools_count_);

    bridge_status_ = memnew(Label);
    bridge_status_->set_text("Bridge: Disconnected");
    status_row->add_child(bridge_status_);

    content->add_child(memnew(HSeparator));

    // ── Client Config Section ──
    Label *client_label = memnew(Label);
    client_label->set_text("Client Config");
    client_label->add_theme_font_size_override("font_size", 16);
    content->add_child(client_label);

    HBoxContainer *client_row = memnew(HBoxContainer);
    content->add_child(client_row);

    client_selector_ = memnew(OptionButton);
    client_selector_->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    populate_client_list();
    client_row->add_child(client_selector_);

    generate_btn_ = memnew(Button);
    generate_btn_->set_text("Generate");
    client_row->add_child(generate_btn_);

    HBoxContainer *btn_row = memnew(HBoxContainer);
    content->add_child(btn_row);

    copy_btn_ = memnew(Button);
    copy_btn_->set_text("Copy");
    copy_btn_->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    btn_row->add_child(copy_btn_);

    // Config preview always visible
    config_preview_ = memnew(RichTextLabel);
    config_preview_->set_custom_minimum_size(Vector2(0, 200));
    config_preview_->set_use_bbcode(false);
    config_preview_->set_selection_enabled(true);
    config_preview_->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    config_preview_->set_v_size_flags(Control::SIZE_EXPAND_FILL);
    content->add_child(config_preview_);

    content->add_child(memnew(HSeparator));

    // ── Settings Section ──
    Label *settings_label = memnew(Label);
    settings_label->set_text("Settings");
    settings_label->add_theme_font_size_override("font_size", 16);
    content->add_child(settings_label);

    GridContainer *port_grid = memnew(GridContainer);
    port_grid->set_columns(2);
    content->add_child(port_grid);

    port_grid->add_child(memnew(Label)); // empty cell
    port_grid->add_child(memnew(Label)); // empty cell

    Label *http_port_label = memnew(Label);
    http_port_label->set_text("HTTP Port");
    port_grid->add_child(http_port_label);

    http_port_spin_ = memnew(SpinBox);
    http_port_spin_->set_min(1024);
    http_port_spin_->set_max(65535);
    http_port_spin_->set_value(9600);
    port_grid->add_child(http_port_spin_);

    Label *bridge_port_label = memnew(Label);
    bridge_port_label->set_text("Bridge Port");
    port_grid->add_child(bridge_port_label);

    bridge_port_spin_ = memnew(SpinBox);
    bridge_port_spin_->set_min(1024);
    bridge_port_spin_->set_max(65535);
    bridge_port_spin_->set_value(9601);
    port_grid->add_child(bridge_port_spin_);

    HBoxContainer *bind_row = memnew(HBoxContainer);
    content->add_child(bind_row);

    Label *bind_label = memnew(Label);
    bind_label->set_text("Bind");
    bind_row->add_child(bind_label);

    bind_mode_ = memnew(OptionButton);
    bind_mode_->add_item("Localhost");
    bind_mode_->add_item("All Interfaces");
    bind_mode_->add_item("Custom");
    bind_mode_->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    bind_row->add_child(bind_mode_);

    custom_bind_addr_ = memnew(LineEdit);
    custom_bind_addr_->set_text("127.0.0.1");
    custom_bind_addr_->set_visible(false);
    custom_bind_addr_->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    bind_row->add_child(custom_bind_addr_);

    HBoxContainer *restart_row = memnew(HBoxContainer);
    content->add_child(restart_row);

    apply_restart_btn_ = memnew(Button);
    apply_restart_btn_->set_text("Apply & Restart");
    apply_restart_btn_->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    restart_row->add_child(apply_restart_btn_);

    force_restart_btn_ = memnew(Button);
    force_restart_btn_->set_text("Force Restart");
    force_restart_btn_->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    restart_row->add_child(force_restart_btn_);

    content->add_child(memnew(HSeparator));

    // ── Console Settings Section ──
    Label *console_label = memnew(Label);
    console_label->set_text("Console Settings");
    console_label->add_theme_font_size_override("font_size", 16);
    content->add_child(console_label);

    GridContainer *console_grid = memnew(GridContainer);
    console_grid->set_columns(2);
    content->add_child(console_grid);

    Label *max_entries_label = memnew(Label);
    max_entries_label->set_text("Max Entries");
    console_grid->add_child(max_entries_label);

    max_entries_spin_ = memnew(SpinBox);
    max_entries_spin_->set_min(100);
    max_entries_spin_->set_max(10000);
    max_entries_spin_->set_value(500);
    max_entries_spin_->set_step(100);
    console_grid->add_child(max_entries_spin_);

    Label *log_dir_label = memnew(Label);
    log_dir_label->set_text("Log Dir");
    console_grid->add_child(log_dir_label);

    log_dir_edit_ = memnew(LineEdit);
    log_dir_edit_->set_text("res://.mcp_logs");
    console_grid->add_child(log_dir_edit_);

    // ── Connect signals ──
    client_selector_->connect("item_selected", Callable(this, "_on_client_changed"));
    generate_btn_->connect("pressed", Callable(this, "_on_generate_pressed"));
    copy_btn_->connect("pressed", Callable(this, "_on_copy_pressed"));
    apply_restart_btn_->connect("pressed", Callable(this, "_on_apply_restart_pressed"));
    force_restart_btn_->connect("pressed", Callable(this, "_on_force_restart_pressed"));
    bind_mode_->connect("item_selected", Callable(this, "_on_bind_mode_changed"));
    max_entries_spin_->connect("value_changed", Callable(this, "_on_max_entries_changed"));
    log_dir_edit_->connect("text_changed", Callable(this, "_on_log_dir_changed"));
}

McpDock::~McpDock() = default;

void McpDock::set_plugin(McpEditorPlugin *p) {
    plugin_ = p;
    http_port_spin_->set_value(p->http_port());
    bridge_port_spin_->set_value(p->bridge_port());
    if (p->http_host() == "0.0.0.0") {
        bind_mode_->select(1);
    } else if (p->http_host() != "127.0.0.1") {
        bind_mode_->select(2);
        custom_bind_addr_->set_text(p->http_host());
        custom_bind_addr_->set_visible(true);
    }
}

// ---------------------------------------------------------------------
// _bind_methods
// ---------------------------------------------------------------------

void McpDock::_bind_methods() {
    ClassDB::bind_method(D_METHOD("_on_generate_pressed"), &McpDock::_on_generate_pressed);
    ClassDB::bind_method(D_METHOD("_on_client_changed", "index"), &McpDock::_on_client_changed);
    ClassDB::bind_method(D_METHOD("_on_copy_pressed"), &McpDock::_on_copy_pressed);
    ClassDB::bind_method(D_METHOD("_on_apply_restart_pressed"), &McpDock::_on_apply_restart_pressed);
    ClassDB::bind_method(D_METHOD("_on_force_restart_pressed"), &McpDock::_on_force_restart_pressed);
    ClassDB::bind_method(D_METHOD("_on_bind_mode_changed", "index"), &McpDock::_on_bind_mode_changed);
    ClassDB::bind_method(D_METHOD("_on_max_entries_changed", "value"), &McpDock::_on_max_entries_changed);
    ClassDB::bind_method(D_METHOD("_on_log_dir_changed", "text"), &McpDock::_on_log_dir_changed);
}

// ---------------------------------------------------------------------
// 客户端列表
// ---------------------------------------------------------------------

void McpDock::populate_client_list() {
    int count;
    const ClientEntry *entries = get_entries(count);
    for (int i = 0; i < count; i++) {
        client_selector_->add_item(String(entries[i].display_name));
    }
}

String McpDock::get_selected_client() const {
    int idx = client_selector_->get_selected();
    int count;
    const ClientEntry *entries = get_entries(count);
    if (idx >= 0 && idx < count) return String(entries[idx].name);
    return "claude_code";
}

// ---------------------------------------------------------------------
// 回调
// ---------------------------------------------------------------------

void McpDock::_on_generate_pressed() {
    if (!registry_) return;
    Dictionary args;
    args["client"] = get_selected_client();
    args["write_to_project"] = true;
    Dictionary result = registry_->execute("generate_client_config", args);
    if (result.has("data")) {
        Dictionary data = result["data"];
        last_config_content_ = data.get("config_content", "");
        config_preview_->set_text(last_config_content_);
    }
}

void McpDock::_on_client_changed(int index) {
    refresh_preview();
}

void McpDock::refresh_preview() {
    if (!registry_) return;
    Dictionary args;
    args["client"] = get_selected_client();
    args["write_to_project"] = false;
    Dictionary result = registry_->execute("generate_client_config", args);
    if (result.has("data")) {
        Dictionary data = result["data"];
        last_config_content_ = data.get("config_content", "");
        config_preview_->set_text(last_config_content_);
    }
}

void McpDock::_on_copy_pressed() {
    if (!last_config_content_.is_empty()) {
        DisplayServer::get_singleton()->clipboard_set(last_config_content_);
    }
}

void McpDock::_on_apply_restart_pressed() {
    if (!plugin_) return;
    int http_port = (int)http_port_spin_->get_value();
    int bridge_port = (int)bridge_port_spin_->get_value();
    int bind_idx = bind_mode_->get_selected();
    String host;
    if (bind_idx == 1) {
        host = "0.0.0.0";
    } else if (bind_idx == 2) {
        host = custom_bind_addr_->get_text();
    } else {
        host = "127.0.0.1";
    }
    plugin_->set_http_port(http_port);
    plugin_->set_bridge_port(bridge_port);
    plugin_->set_http_host(host);
    plugin_->save_config();
    plugin_->restart_server(false);
}

void McpDock::_on_force_restart_pressed() {
    if (!plugin_) return;
    int http_port = (int)http_port_spin_->get_value();
    int bridge_port = (int)bridge_port_spin_->get_value();
    int bind_idx = bind_mode_->get_selected();
    String host;
    if (bind_idx == 1) {
        host = "0.0.0.0";
    } else if (bind_idx == 2) {
        host = custom_bind_addr_->get_text();
    } else {
        host = "127.0.0.1";
    }
    plugin_->set_http_port(http_port);
    plugin_->set_bridge_port(bridge_port);
    plugin_->set_http_host(host);
    plugin_->save_config();
    plugin_->restart_server(true);
}

void McpDock::_on_bind_mode_changed(int index) {
    custom_bind_addr_->set_visible(index == 2);
}

void McpDock::_on_max_entries_changed(double value) {
    if (logger_) {
        logger_->set_max_entries(static_cast<int>(value));
    }
}

void McpDock::_on_log_dir_changed(const String &text) {
    if (logger_) {
        logger_->set_log_dir(text);
    }
}

// ---------------------------------------------------------------------
// 状态更新
// ---------------------------------------------------------------------

void McpDock::update_status() {
    if (!registry_) return;

    int builtin = registry_->builtin_tool_count();
    int custom = registry_->custom_tool_count();
    tools_count_->set_text(String("Tools: ") + String::num_int64(builtin + custom));

    if (plugin_ && plugin_->is_started()) {
        status_icon_->set_text("[ON]");
        status_icon_->add_theme_color_override("font_color", Color(0.2f, 0.9f, 0.2f));
    } else {
        status_icon_->set_text("[OFF]");
        status_icon_->add_theme_color_override("font_color", Color(0.9f, 0.2f, 0.2f));
    }

    EditorInterface *ei = EditorInterface::get_singleton();
    if (ei && ei->is_playing_scene()) {
        bridge_status_->set_text("Bridge: Connected");
        bridge_status_->add_theme_color_override("font_color", Color(0.2f, 0.9f, 0.2f));
    } else {
        bridge_status_->set_text("Bridge: Disconnected");
        bridge_status_->add_theme_color_override("font_color", Color(0.9f, 0.2f, 0.2f));
    }
}

// ---------------------------------------------------------------------
// _process
// ---------------------------------------------------------------------

void McpDock::_process(double delta) {
    time_since_update_ += delta;
    if (time_since_update_ >= 1.0) {
        time_since_update_ = 0.0;
        update_status();
    }
}

} // namespace godot_mcp