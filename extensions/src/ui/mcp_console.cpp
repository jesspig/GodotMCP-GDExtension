// =====================================================================
// mcp_console.cpp — MCP Console 底部面板
// =====================================================================

#include "mcp_console.hpp"
#include "client_config_registry.hpp"
#include "editor_plugin.hpp"
#include "mcp_logger.hpp"

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/editor_settings.hpp>
#include <godot_cpp/classes/code_highlighter.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/h_separator.hpp>
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/classes/style_box_flat.hpp>
#include <godot_cpp/core/class_db.hpp>

using namespace godot;

namespace godot_mcp {

enum ContextMenuId {
    MENU_COPY_ENTRY = 0,
    MENU_COPY_ARGS,
    MENU_COPY_RESULT,
};

// =====================================================================
// 构造 / 析构
// =====================================================================

McpConsole::McpConsole() {
    set_auto_translate(false);
    set_process(true);

    // ── Status Bar ───────────────────────────────────────────────────
    HBoxContainer *status_bar = memnew(HBoxContainer);
    status_bar->add_theme_constant_override("separation", 16);
    add_child(status_bar);

    status_icon_ = memnew(Label);
    status_icon_->set_text("[OFF]");
    status_icon_->add_theme_color_override("font_color", Color(0.9f, 0.2f, 0.2f));
    status_icon_->add_theme_font_size_override("font_size", 14);
    status_bar->add_child(status_icon_);

    tools_count_ = memnew(Label);
    tools_count_->set_text("Tools: 0");
    tools_count_->add_theme_font_size_override("font_size", 14);
    status_bar->add_child(tools_count_);

    bridge_status_ = memnew(Label);
    bridge_status_->set_text("Bridge: Disconnected");
    bridge_status_->add_theme_font_size_override("font_size", 14);
    status_bar->add_child(bridge_status_);

    Control *status_spacer = memnew(Control);
    status_spacer->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    status_bar->add_child(status_spacer);

    add_child(memnew(HSeparator));

    // ── Toolbar ──────────────────────────────────────────────────────
    HBoxContainer *toolbar = memnew(HBoxContainer);
    add_child(toolbar);

    expand_btn_ = memnew(Button);
    expand_btn_->set_text("Expand All");
    expand_btn_->set_disabled(true);
    toolbar->add_child(expand_btn_);

    collapse_btn_ = memnew(Button);
    collapse_btn_->set_text("Collapse All");
    collapse_btn_->set_disabled(true);
    toolbar->add_child(collapse_btn_);

    Control *spacer = memnew(Control);
    spacer->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    toolbar->add_child(spacer);

    count_label_ = memnew(Label);
    count_label_->set_text("0 entries");
    toolbar->add_child(count_label_);

    clear_btn_ = memnew(Button);
    clear_btn_->set_text("Clear");
    clear_btn_->set_disabled(true);
    toolbar->add_child(clear_btn_);

    toolbar->add_child(memnew(VSeparator));

    config_client_selector_ = memnew(OptionButton);
    int cc;
    const ClientEntry *ce = get_entries(cc);
    for (int i = 0; i < cc; i++) {
        config_client_selector_->add_item(String(ce[i].display_name));
    }
    config_client_selector_->set_h_size_flags(Control::SIZE_SHRINK_CENTER);
    toolbar->add_child(config_client_selector_);

    config_apply_btn_ = memnew(Button);
    config_apply_btn_->set_text("Configure");
    toolbar->add_child(config_apply_btn_);

    auto_scroll_chk_ = memnew(CheckBox);
    auto_scroll_chk_->set_text("Auto-scroll");
    auto_scroll_chk_->set_pressed(true);
    toolbar->add_child(auto_scroll_chk_);

    // ── Custom column header (visible draggable divider) ─────────────
    HBoxContainer *header = memnew(HBoxContainer);
    header->set_alignment(BoxContainer::ALIGNMENT_BEGIN);

    time_header_ = memnew(Label);
    time_header_->set_text("Time");
    time_header_->set_custom_minimum_size(Vector2(170, 0));
    time_header_->set_h_size_flags(Control::SIZE_SHRINK_BEGIN);
    header->add_child(time_header_);

    drag_area_ = memnew(Panel);
    drag_area_->set_custom_minimum_size(Vector2(2, 0));
    drag_area_->set_mouse_filter(Control::MOUSE_FILTER_STOP);
    drag_area_->set_default_cursor_shape(Control::CURSOR_HSIZE);
    {
        Ref<StyleBoxFlat> dg_bg;
        dg_bg.instantiate();
        dg_bg->set_bg_color(Color(0.12f, 0.12f, 0.12f));
        drag_area_->add_theme_stylebox_override("panel", dg_bg);
    }
    header->add_child(drag_area_);

    Label *tool_header = memnew(Label);
    tool_header->set_text("Tool");
    tool_header->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    header->add_child(tool_header);

    add_child(header);

    drag_area_->connect("gui_input", Callable(this, "_on_drag_area_gui_input"));

    // ── Main split: Tree (top) | Detail (bottom) ─────────────────────
    VSplitContainer *split = memnew(VSplitContainer);
    split->set_v_size_flags(Control::SIZE_EXPAND_FILL);
    split->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    split->set_split_offset(-200);
    add_child(split);

    PanelContainer *tree_panel = memnew(PanelContainer);
    tree_panel->set_v_size_flags(Control::SIZE_EXPAND_FILL);
    {
        Ref<StyleBoxFlat> bg;
        bg.instantiate();
        bg->set_bg_color(Color(0.12f, 0.12f, 0.12f));
        bg->set_content_margin_all(2);
        bg->set_corner_radius(Corner::CORNER_TOP_LEFT,3);
        bg->set_corner_radius(Corner::CORNER_TOP_RIGHT,3);
        bg->set_corner_radius(Corner::CORNER_BOTTOM_RIGHT,3);
        bg->set_corner_radius(Corner::CORNER_BOTTOM_LEFT,3);
        tree_panel->add_theme_stylebox_override("panel", bg);
    }
    split->add_child(tree_panel);

    log_tree_ = memnew(Tree);
    log_tree_->set_v_size_flags(Control::SIZE_EXPAND_FILL);
    log_tree_->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    log_tree_->set_hide_root(true);
    log_tree_->set_allow_reselect(true);
    log_tree_->set_allow_rmb_select(true);

    tree_panel->add_child(log_tree_);

    setup_tree_columns();

    // Detail panel: Request (left) | Response (right)
    VBoxContainer *detail = memnew(VBoxContainer);
    detail->set_custom_minimum_size(Vector2(0, 150));
    detail->add_child(memnew(HSeparator));
    split->add_child(detail);

    HBoxContainer *hdr = memnew(HBoxContainer);
    detail->add_child(hdr);

    Label *req_hdr = memnew(Label);
    req_hdr->set_text("Request");
    req_hdr->add_theme_font_size_override("font_size", 14);
    hdr->add_child(req_hdr);

    Control *hdr_spacer = memnew(Control);
    hdr_spacer->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    hdr->add_child(hdr_spacer);

    Label *res_hdr = memnew(Label);
    res_hdr->set_text("Response");
    res_hdr->add_theme_font_size_override("font_size", 14);
    hdr->add_child(res_hdr);

    HSplitContainer *content = memnew(HSplitContainer);
    content->set_v_size_flags(Control::SIZE_EXPAND_FILL);
    content->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    detail->add_child(content);

    req_view_ = memnew(CodeEdit);
    req_view_->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    req_view_->set_v_size_flags(Control::SIZE_EXPAND_FILL);
    req_view_->set_editable(false);
    req_view_->set_selecting_enabled(true);
    req_view_->set_draw_line_numbers(true);
    req_view_->set_line_numbers_zero_padded(false);
    req_view_->set_line_wrapping_mode(TextEdit::LINE_WRAPPING_NONE);
    content->add_child(req_view_);

    res_view_ = memnew(CodeEdit);
    res_view_->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    res_view_->set_v_size_flags(Control::SIZE_EXPAND_FILL);
    res_view_->set_editable(false);
    res_view_->set_selecting_enabled(true);
    res_view_->set_draw_line_numbers(true);
    res_view_->set_line_numbers_zero_padded(false);
    res_view_->set_line_wrapping_mode(TextEdit::LINE_WRAPPING_NONE);
    content->add_child(res_view_);

    // ── JSON syntax highlighter (matching Godot editor JSON highlighter) ─
    {
        Ref<CodeHighlighter> hl;
        hl.instantiate();
        hl->clear_keyword_colors();
        hl->clear_member_keyword_colors();
        hl->clear_color_regions();

        EditorInterface *ei = EditorInterface::get_singleton();
        if (ei) {
            Ref<EditorSettings> s = ei->get_editor_settings();
            if (s.is_valid()) {
                hl->set_symbol_color(s->get_setting("text_editor/theme/highlighting/symbol_color"));
                hl->set_number_color(s->get_setting("text_editor/theme/highlighting/number_color"));
                Color sc = s->get_setting("text_editor/theme/highlighting/string_color");
                hl->add_color_region("\"", "\"", sc);
            }
        }

        req_view_->set_syntax_highlighter(hl);
        res_view_->set_syntax_highlighter(hl);
    }

    // ── Context menu ─────────────────────────────────────────────────
    context_menu_ = memnew(PopupMenu);
    add_child(context_menu_);
    context_menu_->add_item("Copy Entry", MENU_COPY_ENTRY);
    context_menu_->add_item("Copy Args", MENU_COPY_ARGS);
    context_menu_->add_item("Copy Result", MENU_COPY_RESULT);

    // ── Signals ──────────────────────────────────────────────────────
    expand_btn_->connect("pressed", Callable(this, "_on_expand_all"));
    collapse_btn_->connect("pressed", Callable(this, "_on_collapse_all"));
    clear_btn_->connect("pressed", Callable(this, "_on_clear_pressed"));
    auto_scroll_chk_->connect("toggled", Callable(this, "_on_auto_scroll_toggled"));
    config_apply_btn_->connect("pressed", Callable(this, "_on_config_apply_pressed"));
    log_tree_->connect("item_selected", Callable(this, "_on_item_selected"));
    log_tree_->connect("item_activated", Callable(this, "_on_item_activated"));
    log_tree_->connect("item_mouse_selected", Callable(this, "_on_item_mouse_selected"));
    context_menu_->connect("id_pressed", Callable(this, "_on_context_menu_id_pressed"));
}

McpConsole::~McpConsole() = default;

// =====================================================================
// 列配置
// =====================================================================

void McpConsole::setup_tree_columns() {
    log_tree_->set_columns(2);
    log_tree_->set_column_titles_visible(false);

    log_tree_->set_column_expand(0, false);
    log_tree_->set_column_expand(1, true);

    log_tree_->set_column_custom_minimum_width(0, 170);
    log_tree_->set_column_custom_minimum_width(1, 250);
}

// =====================================================================
// Logger 绑定
// =====================================================================

void McpConsole::set_logger(McpLogger *logger) {
    logger_ = logger;
}

// =====================================================================
// 格式化
// =====================================================================

String McpConsole::fmt_tool(const McpLogger::LogEntry &e) {
    String status = e.success ? "OK" : "FAIL";
    return String("[") + status + String("] ") + e.tool_name
        + String("  ") + String::num(e.duration_ms, 1) + String(" ms");
}

String McpConsole::json_compact(const Variant &v) {
    return JSON::stringify(v, "");
}

String McpConsole::format_json(const Variant &v) {
    if (v.get_type() == Variant::DICTIONARY) {
        Dictionary d = v;
        if (d.is_empty()) return "{}";
    }
    if (v.get_type() == Variant::ARRAY) {
        Array a = v;
        if (a.is_empty()) return "[]";
    }
    return JSON::stringify(v, "  ");
}

String McpConsole::format_entry(const McpLogger::LogEntry &entry, bool with_children) {
    String text;
    text += String("[") + entry.timestamp + String("] ");
    text += fmt_tool(entry);
    if (with_children) {
        text += String("\n  args: ") + json_compact(entry.args);
        text += String("\n  result: ") + json_compact(entry.result);
    }
    return text;
}

// =====================================================================
// 添加行
// =====================================================================

void McpConsole::add_tree_entry(const McpLogger::LogEntry &entry, int index) {
    TreeItem *root = log_tree_->get_root();
    if (!root) {
        root = log_tree_->create_item();
    }

    TreeItem *item = log_tree_->create_item(root);
    item->set_text(0, entry.timestamp);
    item->set_text(1, fmt_tool(entry));
    item->set_collapsed(true);
    item->set_metadata(0, index);

    Color tint = entry.success ? Color(0.3f, 0.9f, 0.3f) : Color(1.0f, 0.3f, 0.3f);
    item->set_custom_color(1, tint);

    TreeItem *req_item = log_tree_->create_item(item);
    req_item->set_text(0, "Request");
    req_item->set_text(1, json_compact(entry.args));

    TreeItem *res_item = log_tree_->create_item(item);
    res_item->set_text(0, "Response");
    res_item->set_text(1, json_compact(entry.result));

    if (auto_scroll_) {
        log_tree_->scroll_to_item(item);
    }
}

// =====================================================================
// 重建
// =====================================================================

void McpConsole::rebuild_log() {
    log_tree_->clear();
    for (size_t i = 0; i < logged_entries_.size(); i++) {
        add_tree_entry(logged_entries_[i], static_cast<int>(i));
    }
    update_toolbar_state();
    update_detail();
}

// =====================================================================
// on_log_appended
// =====================================================================

void McpConsole::on_log_appended(const McpLogger::LogEntry &entry) {
    logged_entries_.push_back(entry);
    while (logged_entries_.size() > kMaxVisible) {
        logged_entries_.pop_front();
        TreeItem *root = log_tree_->get_root();
        if (root) {
            TreeItem *oldest = root->get_first_child();
            if (oldest) {
                root->remove_child(oldest);
                memdelete(oldest);
            }
        }
    }
    add_tree_entry(entry, static_cast<int>(logged_entries_.size()) - 1);
    update_toolbar_state();
}

// =====================================================================
// 可见计数
// =====================================================================

int McpConsole::visible_count() const {
    TreeItem *root = log_tree_->get_root();
    if (!root) return 0;
    return root->get_child_count();
}

void McpConsole::update_toolbar_state() {
    int vc = visible_count();
    count_label_->set_text(String::num_int64(vc) + " entries");
    bool any = vc > 0;
    expand_btn_->set_disabled(!any);
    collapse_btn_->set_disabled(!any);
    clear_btn_->set_disabled(!any);
}

// =====================================================================
// 详情面板
// =====================================================================

void McpConsole::update_detail() {
    TreeItem *item = log_tree_->get_selected();
    if (!item) {
        req_view_->set_text("");
        res_view_->set_text("");
        return;
    }
    Variant meta = item->get_metadata(0);
    if (meta.get_type() != Variant::INT) {
        item = item->get_parent();
        if (!item) { req_view_->set_text(""); res_view_->set_text(""); return; }
        meta = item->get_metadata(0);
    }
    if (meta.get_type() != Variant::INT) {
        req_view_->set_text("");
        res_view_->set_text("");
        return;
    }
    int idx = static_cast<int>(meta);
    if (idx < 0 || static_cast<size_t>(idx) >= logged_entries_.size()) return;

    const McpLogger::LogEntry &entry = logged_entries_[idx];
    req_view_->set_text(format_json(entry.args));
    res_view_->set_text(format_json(entry.result));
}

// =====================================================================
// 选中 / 双击
// =====================================================================

void McpConsole::_on_item_selected() {
    update_detail();
}

void McpConsole::_on_item_activated() {
    TreeItem *selected = log_tree_->get_selected();
    if (!selected) return;
    if (selected->get_first_child()) {
        selected->set_collapsed(!selected->is_collapsed());
    }
}

// =====================================================================
// 右键菜单
// =====================================================================

void McpConsole::_on_item_mouse_selected(const Vector2 &pos, int32_t button) {
    if (button != MOUSE_BUTTON_RIGHT) return;
    TreeItem *item = log_tree_->get_selected();
    if (!item) return;

    Variant meta = item->get_metadata(0);
    if (meta.get_type() != Variant::INT) {
        item = item->get_parent();
        if (!item) return;
        meta = item->get_metadata(0);
    }
    if (meta.get_type() != Variant::INT) return;

    context_menu_->set_meta("entry_index", meta);
    context_menu_->set_position(get_screen_position() + pos);
    context_menu_->popup();
}

void McpConsole::_on_context_menu_id_pressed(int32_t id) {
    Variant meta = context_menu_->get_meta("entry_index");
    if (meta.get_type() != Variant::INT) return;
    int idx = static_cast<int>(meta);
    if (idx < 0 || static_cast<size_t>(idx) >= logged_entries_.size()) return;

    const McpLogger::LogEntry &entry = logged_entries_[idx];
    String text;
    switch (id) {
        case MENU_COPY_ENTRY: text = format_entry(entry, true); break;
        case MENU_COPY_ARGS:  text = JSON::stringify(entry.args, "  "); break;
        case MENU_COPY_RESULT:text = JSON::stringify(entry.result, "  "); break;
    }
    if (!text.is_empty()) {
        if (auto *ds = DisplayServer::get_singleton()) {
            ds->clipboard_set(text);
        }
    }
}

// =====================================================================
// 刷新
// =====================================================================

void McpConsole::refresh() {
    if (!logger_) return;
    logged_entries_.clear();
    const std::deque<McpLogger::LogEntry> &entries = logger_->entries();
    for (size_t i = 0; i < entries.size(); i++) {
        logged_entries_.push_back(entries[i]);
    }
    rebuild_log();
}

// =====================================================================
// 展开 / 折叠
// =====================================================================

void McpConsole::_on_expand_all() {
    TreeItem *root = log_tree_->get_root();
    if (!root) return;
    TreeItem *item = root->get_first_child();
    while (item) {
        item->set_collapsed(false);
        item = item->get_next();
    }
}

void McpConsole::_on_collapse_all() {
    TreeItem *root = log_tree_->get_root();
    if (!root) return;
    TreeItem *item = root->get_first_child();
    while (item) {
        item->set_collapsed(true);
        item = item->get_next();
    }
}

// =====================================================================
// 清除
// =====================================================================

void McpConsole::_on_clear_pressed() {
    log_tree_->clear();
    logged_entries_.clear();
    count_label_->set_text("0 entries");
    expand_btn_->set_disabled(true);
    collapse_btn_->set_disabled(true);
    clear_btn_->set_disabled(true);
    req_view_->set_text("");
    res_view_->set_text("");
    if (logger_) {
        logger_->clear();
    }
}

void McpConsole::_on_auto_scroll_toggled(bool pressed) {
    auto_scroll_ = pressed;
}

// =====================================================================
// 客户端配置生成
// =====================================================================

void McpConsole::_on_config_apply_pressed() {
    int sel = config_client_selector_->get_selected();
    int count;
    const ClientEntry *entries = get_entries(count);
    if (sel < 0 || sel >= count) return;

    int port = 9600;
    ProjectSettings *ps = ProjectSettings::get_singleton();
    if (ps) {
        Variant port_v = ps->get_setting("godot_mcp/http_port");
        if (port_v.get_type() == Variant::INT) {
            port = static_cast<int>(static_cast<int64_t>(port_v));
        }
    }
    String url = String("http://127.0.0.1:") + String::num_int64(port) + String("/mcp");
    String content = entries[sel].generator(url);

    if (auto *ds = DisplayServer::get_singleton()) {
        ds->clipboard_set(content);
    }

    count_label_->set_text(String("Config for ") + String(entries[sel].display_name) + String(" copied"));
    count_label_->add_theme_color_override("font_color", Color(0.2f, 0.9f, 0.2f));
}

// =====================================================================
// 列宽拖拽
// =====================================================================

void McpConsole::_on_drag_area_gui_input(const Ref<InputEvent> &event) {
    Ref<InputEventMouseMotion> mm = event;
    if (mm.is_valid() && godot::Input::get_singleton()->is_mouse_button_pressed(MOUSE_BUTTON_LEFT)) {
        float w = get_global_mouse_position().x - time_header_->get_global_position().x;
        if (w < 60) w = 60;
        time_header_->set_custom_minimum_size(Vector2(w, 0));
        log_tree_->set_column_custom_minimum_width(0, static_cast<int>(w));
    }
}

// =====================================================================
// 状态更新
// =====================================================================

void McpConsole::update_status() {
    if (!plugin_ || !registry_) return;

    int builtin = registry_->builtin_tool_count();
    int custom = registry_->custom_tool_count();
    String tools_text = String("Tools: ") + String::num_int64(builtin + custom);
    if (tools_text != cached_tools_text_) {
        cached_tools_text_ = tools_text;
        tools_count_->set_text(tools_text);
    }

    if (plugin_->is_started()) {
        int cfg_port = plugin_->http_port();
        int actual_port = plugin_->actual_http_port();
        String st;
        if (actual_port != cfg_port) {
            st = String("[ON :") + String::num_int64(actual_port) +
                 String("] (cfg ") + String::num_int64(cfg_port) +
                 String(" in use)");
        } else {
            st = String("[ON :") + String::num_int64(actual_port) + String("]");
        }
        Color sc(0.2f, 0.9f, 0.2f);
        if (st != cached_status_text_ || sc != cached_status_color_) {
            cached_status_text_ = st;
            cached_status_color_ = sc;
            status_icon_->set_text(st);
            status_icon_->add_theme_color_override("font_color", sc);
        }
    } else {
        String st = "[OFF]";
        Color sc(0.9f, 0.2f, 0.2f);
        if (st != cached_status_text_ || sc != cached_status_color_) {
            cached_status_text_ = st;
            cached_status_color_ = sc;
            status_icon_->set_text(st);
            status_icon_->add_theme_color_override("font_color", sc);
        }
    }

    if (plugin_->is_bridge_connected()) {
        String bt = "Bridge: Connected";
        Color bc(0.2f, 0.9f, 0.2f);
        if (bt != cached_bridge_text_ || bc != cached_bridge_color_) {
            cached_bridge_text_ = bt;
            cached_bridge_color_ = bc;
            bridge_status_->set_text(bt);
            bridge_status_->add_theme_color_override("font_color", bc);
        }
    } else {
        String bt = "Bridge: Disconnected";
        Color bc(0.9f, 0.2f, 0.2f);
        if (bt != cached_bridge_text_ || bc != cached_bridge_color_) {
            cached_bridge_text_ = bt;
            cached_bridge_color_ = bc;
            bridge_status_->set_text(bt);
            bridge_status_->add_theme_color_override("font_color", bc);
        }
    }
}

void McpConsole::_process(double delta) {
    time_since_status_update_ += delta;
    if (time_since_status_update_ >= 1.0) {
        time_since_status_update_ = 0.0;
        update_status();
    }
}

// =====================================================================
// Godot 绑定
// =====================================================================

void McpConsole::_bind_methods() {
    ClassDB::bind_method(D_METHOD("_on_clear_pressed"), &McpConsole::_on_clear_pressed);
    ClassDB::bind_method(D_METHOD("_on_expand_all"), &McpConsole::_on_expand_all);
    ClassDB::bind_method(D_METHOD("_on_collapse_all"), &McpConsole::_on_collapse_all);
    ClassDB::bind_method(D_METHOD("_on_auto_scroll_toggled", "pressed"), &McpConsole::_on_auto_scroll_toggled);
    ClassDB::bind_method(D_METHOD("_on_config_apply_pressed"), &McpConsole::_on_config_apply_pressed);
    ClassDB::bind_method(D_METHOD("_on_item_selected"), &McpConsole::_on_item_selected);
    ClassDB::bind_method(D_METHOD("_on_item_activated"), &McpConsole::_on_item_activated);
    ClassDB::bind_method(D_METHOD("_on_item_mouse_selected", "pos", "button"), &McpConsole::_on_item_mouse_selected);
    ClassDB::bind_method(D_METHOD("_on_context_menu_id_pressed", "id"), &McpConsole::_on_context_menu_id_pressed);
    ClassDB::bind_method(D_METHOD("_on_drag_area_gui_input", "event"), &McpConsole::_on_drag_area_gui_input);
}

} // namespace godot_mcp
