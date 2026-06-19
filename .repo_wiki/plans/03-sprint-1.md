# Sprint 1 开发文档

> 精确到代码级的实现指南。每个步骤包含：修改位置（文件:行号）、完整代码、集成点、验证方法。

---

## Q1: 渐进式工具披露开关

### 步骤 1: HandlerRegistry 新增方法

**文件**: `extensions/src/server/registry/handler_registry.hpp:49` 附近

在 `get_always_on_tools()` 声明之后添加：

```cpp
[[nodiscard]] godot::Array get_all_tools_for_list() const;
```

**文件**: `extensions/src/server/registry/handler_registry.cpp:319` 附近

在 `get_always_on_tools()` 实现之后添加：

```cpp
Array HandlerRegistry::get_all_tools_for_list() const {
    Array result;
    for (const auto &[name, info] : tool_info_) {
        if (!info.enabled) continue;
        result.push_back(make_tool_entry(name, info));
    }
    return result;
}
```

### 步骤 2: McpHandler 新增标志和 setter

**文件**: `extensions/src/server/mcp/mcp_handler.hpp`

在 `McpHandler` 类的 `private` 区域（`log_level_` 成员之后，约第 96 行）添加：

```cpp
bool progressive_disclosure_ = true;
```

在 `public` 区域（`set_log_callback` 之后）添加：

```cpp
void set_progressive_disclosure(bool enabled) { progressive_disclosure_ = enabled; }
bool is_progressive_disclosure() const noexcept { return progressive_disclosure_; }
```

### 步骤 3: McpHandler::handle_tools_list 修改

**文件**: `extensions/src/server/mcp/mcp_handler.cpp:299-308`

将现有实现：

```cpp
Dictionary McpHandler::handle_tools_list(const Dictionary & /*params*/, const Variant &id) {
    if (!registry_) {
        return make_jsonrpc_error(id, kInternalError, "Registry not initialized");
    }
    Array always_on = registry_->get_always_on_tools();

    Dictionary result;
    result["tools"] = always_on;
    return make_jsonrpc_result(id, result);
}
```

替换为：

```cpp
Dictionary McpHandler::handle_tools_list(const Dictionary & /*params*/, const Variant &id) {
    if (!registry_) {
        return make_jsonrpc_error(id, kInternalError, "Registry not initialized");
    }

    Array tools;
    if (progressive_disclosure_) {
        tools = registry_->get_always_on_tools();
    } else {
        tools = registry_->get_all_tools_for_list();
    }

    Dictionary result;
    result["tools"] = tools;
    return make_jsonrpc_result(id, result);
}
```

### 步骤 4: McpEditorPlugin 配置读写

**文件**: `extensions/src/editor_plugin.hpp`

在 `McpEditorPlugin` 类的 `private` 区域（`game_was_running_` 之后）添加：

```cpp
bool progressive_disclosure_ = true;
```

在 `public` 区域添加：

```cpp
void set_progressive_disclosure(bool enabled);
bool is_progressive_disclosure() const noexcept { return progressive_disclosure_; }
```

**文件**: `extensions/src/editor_plugin.cpp`

在 `load_config()` 中（`bridge_port` 读取之后，`if (any_nil)` 之前，约第 69 行）添加：

```cpp
Variant pd_v = ps->get_setting("godot_mcp/progressive_disclosure");
if (pd_v.get_type() == Variant::BOOL) {
    progressive_disclosure_ = static_cast<bool>(pd_v);
} else {
    progressive_disclosure_ = true;
    any_nil = true;
}
```

在 `save_config()` 中（`bridge_port` 写入之后，`ps->save()` 之前）添加：

```cpp
ps->set_setting("godot_mcp/progressive_disclosure", progressive_disclosure_);
```

在 `_enter_tree()` 中（`load_config()` 调用之后，`http_server_.start()` 之前）添加：

```cpp
mcp_handler_.set_progressive_disclosure(progressive_disclosure_);
```

新增方法实现：

```cpp
void McpEditorPlugin::set_progressive_disclosure(bool enabled) {
    progressive_disclosure_ = enabled;
    mcp_handler_.set_progressive_disclosure(enabled);
    save_config();
}
```

### 步骤 5: McpDock UI

**文件**: `extensions/src/ui/mcp_dock.hpp`

在 `McpDock` 类的 `private` 区域（`log_dir_edit_` 之后）添加：

```cpp
godot::CheckBox *progressive_disclosure_chk_ = nullptr;
void _on_progressive_disclosure_toggled(bool pressed);
```

**文件**: `extensions/src/ui/mcp_dock.cpp`

在构造函数中（Console Settings 区域之后，信号连接之前）添加 UI 创建：

```cpp
content->add_child(memnew(HSeparator));

Label *advanced_label = memnew(Label);
advanced_label->set_text("Advanced");
advanced_label->add_theme_font_size_override("font_size", 16);
content->add_child(advanced_label);

progressive_disclosure_chk_ = memnew(CheckBox);
progressive_disclosure_chk_->set_text("Progressive Disclosure");
progressive_disclosure_chk_->set_pressed(true);
progressive_disclosure_chk_->set_tooltip_text(
    "When enabled, tools/list returns only meta tools (8).\n"
    "When disabled, returns all tools (153+).");
content->add_child(progressive_disclosure_chk_);
```

在信号连接区域（现有 `connect` 调用之后）添加：

```cpp
progressive_disclosure_chk_->connect("toggled",
    Callable(this, "_on_progressive_disclosure_toggled"));
```

在 `_bind_methods()` 中添加：

```cpp
ClassDB::bind_method(D_METHOD("_on_progressive_disclosure_toggled", "pressed"),
    &McpDock::_on_progressive_disclosure_toggled);
```

在 `set_plugin()` 方法中（现有 `bridge_port_spin_` 设置之后）添加：

```cpp
if (p->is_progressive_disclosure()) {
    progressive_disclosure_chk_->set_pressed(true);
} else {
    progressive_disclosure_chk_->set_pressed(false);
}
```

回调实现：

```cpp
void McpDock::_on_progressive_disclosure_toggled(bool pressed) {
    if (plugin_) {
        plugin_->set_progressive_disclosure(pressed);
    }
}
```

### 验证

1. 编译通过：`uv run python main.py build`
2. 打开 Godot 编辑器，在右侧 Dock 看到 "Advanced" 区域和 "Progressive Disclosure" 复选框
3. 默认勾选 → MCP Inspector `tools/list` 返回 8 个工具
4. 取消勾选 → `tools/list` 返回 153+ 个工具
5. 重启编辑器 → 设置持久化（从 ProjectSettings 读取）

---

## Q2: 破坏性操作确认 UI

### 步骤 1: 创建 PendingDestructiveOp 结构

**新文件**: `extensions/src/server/mcp/pending_operation.hpp`

```cpp
#pragma once

#include <cstdint>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/variant.hpp>

namespace godot_mcp {

struct PendingDestructiveOp {
    godot::Variant jsonrpc_id;
    godot::String tool_name;
    godot::Dictionary arguments;
    uint64_t created_msec = 0;

    enum class State : uint8_t { Pending, Resolved };
    State state = State::Pending;
};

} // namespace godot_mcp
```

### 步骤 2: McpHandler 新增 pending 逻辑

**文件**: `extensions/src/server/mcp/mcp_handler.hpp`

新增 include：

```cpp
#include "pending_operation.hpp"
#include <deque>
```

在 `McpHandler` 类的 `private` 区域添加：

```cpp
std::deque<PendingDestructiveOp> pending_ops_;
bool allow_all_for_session_ = false;

using ConfirmCallback = std::function<void(const PendingDestructiveOp &)>;
ConfirmCallback confirm_callback_;
```

在 `public` 区域添加：

```cpp
void set_confirm_callback(ConfirmCallback cb) { confirm_callback_ = std::move(cb); }
void resolve_pending_op(const godot::Variant &id, bool allow, bool allow_all_for_session);
void check_pending_timeouts(uint64_t timeout_msec);
void reset_session_flags() { allow_all_for_session_ = false; }
int pending_op_count() const noexcept { return static_cast<int>(pending_ops_.size()); }
```

**文件**: `extensions/src/server/mcp/mcp_handler.cpp`

在 `handle_tools_call()` 中（`const Dictionary args = ...` 之后，`Dictionary exec_result = ...` 之前，约第 322 行）插入破坏性检测：

```cpp
    // ── Destructive tool interception ──
    if (registry_) {
        const ToolInfo *info = registry_->find_tool_info(tool_name);
        if (info && info->is_destructive && !allow_all_for_session_) {
            PendingDestructiveOp op;
            op.jsonrpc_id = id;
            op.tool_name = tool_name;
            op.arguments = args;
            op.created_msec = Time::get_singleton()->get_ticks_msec();
            pending_ops_.push_back(std::move(op));

            if (confirm_callback_) {
                confirm_callback_(pending_ops_.back());
            }

            // Return empty Dictionary — no JSON-RPC response yet.
            // Result will be pushed via SSE after user confirms/denies.
            return Dictionary();
        }
    }
    // ── End destructive interception ──
```

在 `mcp_handler.cpp` 末尾（`}` namespace closing 之前）添加新方法：

```cpp
void McpHandler::resolve_pending_op(const Variant &id, bool allow, bool allow_all_for_session) {
    if (allow_all_for_session) {
        allow_all_for_session_ = true;
    }

    for (auto it = pending_ops_.begin(); it != pending_ops_.end(); ++it) {
        // Compare jsonrpc_id — Variant equality works for int/string
        if (it->jsonrpc_id != id) continue;
        if (it->state != PendingDestructiveOp::State::Pending) continue;

        it->state = PendingDestructiveOp::State::Resolved;

        if (allow) {
            // Execute the tool
            Dictionary exec_result = tool_executor_.execute(it->tool_name, it->arguments);

            // Build JSON-RPC response
            Dictionary jsonrpc_resp;
            if (exec_result.has("_exec_error")) {
                const Dictionary err_info = exec_result["_exec_error"];
                jsonrpc_resp = make_jsonrpc_error(it->jsonrpc_id,
                    err_info.get("code", kInternalError),
                    err_info.get("message", "Unknown error"),
                    err_info.get("data", Variant()));
            } else {
                exec_result.erase("_raw_result");
                exec_result.erase("_exec_duration_ms");
                jsonrpc_resp = make_jsonrpc_result(it->jsonrpc_id, exec_result);
            }
            enqueue_event(jsonrpc_resp);
        } else {
            // Deny — return error via SSE
            Dictionary err_resp = make_jsonrpc_error(it->jsonrpc_id, kInvalidRequest,
                String("Destructive tool denied by user: ") + it->tool_name);
            enqueue_event(err_resp);
        }

        pending_ops_.erase(it);
        return;
    }
}

void McpHandler::check_pending_timeouts(uint64_t timeout_msec) {
    const uint64_t now = Time::get_singleton()->get_ticks_msec();
    std::deque<PendingDestructiveOp> still_pending;

    for (auto &op : pending_ops_) {
        if (op.state != PendingDestructiveOp::State::Pending) continue;
        if (now - op.created_msec > timeout_msec) {
            // Timeout — auto-deny
            Dictionary err_resp = make_jsonrpc_error(op.jsonrpc_id, kInvalidRequest,
                String("Destructive tool timed out (no user response): ") + op.tool_name);
            enqueue_event(err_resp);
        } else {
            still_pending.push_back(std::move(op));
        }
    }
    pending_ops_ = std::move(still_pending);
}
```

### 步骤 3: McpConfirmDialog UI

**新文件**: `extensions/src/ui/mcp_confirm_dialog.hpp`

```cpp
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
```

**新文件**: `extensions/src/ui/mcp_confirm_dialog.cpp`

```cpp
#include "mcp_confirm_dialog.hpp"
#include <godot_cpp/classes/json.hpp>

using namespace godot;

namespace godot_mcp {

McpConfirmDialog::McpConfirmDialog() {
    set_title("Destructive Operation - Confirm");
    set_size(Vector2(480, 320));
    set_exclusive(true);
    set_unresizable(false);
    set_min_size(Vector2(360, 240));

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

    // Signals
    deny_btn_->connect("pressed", Callable(this, "_on_deny_pressed"));
    allow_btn_->connect("pressed", Callable(this, "_on_allow_pressed"));
    allow_all_btn_->connect("pressed", Callable(this, "_on_allow_all_pressed"));
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
    popup_centered();
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
```

### 步骤 4: McpEditorPlugin 集成

**文件**: `extensions/src/editor_plugin.hpp`

新增 include：

```cpp
#include "ui/mcp_confirm_dialog.hpp"
#include <optional>
```

在 `McpEditorPlugin` 类的 `private` 区域添加：

```cpp
McpConfirmDialog *confirm_dialog_ = nullptr;
std::optional<PendingDestructiveOp> pending_dialog_op_;
static constexpr uint64_t kConfirmTimeoutMs = 30000;

void _on_confirm_allow(const Variant &id);
void _on_confirm_deny(const Variant &id);
void _on_confirm_allow_all(const Variant &id);
```

**文件**: `extensions/src/editor_plugin.cpp`

在 `_enter_tree()` 中（McpConsole 创建之后，`started_ = true` 之前）添加：

```cpp
// Confirmation dialog for destructive operations
confirm_dialog_ = memnew(McpConfirmDialog);
EditorInterface::get_singleton()->get_base_control()->add_child(confirm_dialog_);
confirm_dialog_->connect("confirmed", Callable(this, "_on_confirm_allow"));
confirm_dialog_->connect("denied", Callable(this, "_on_confirm_deny"));
confirm_dialog_->connect("allow_all_session", Callable(this, "_on_confirm_allow_all"));

mcp_handler_.set_confirm_callback([this](const PendingDestructiveOp &op) {
    pending_dialog_op_ = op;
});
```

在 `_process()` 中（`http_server_.poll()` 之后，`_try_bridge_connect()` 之前）添加：

```cpp
// Show pending confirmation dialog (deferred from poll callback)
if (pending_dialog_op_.has_value()) {
    confirm_dialog_->show_for_op(
        pending_dialog_op_->jsonrpc_id,
        pending_dialog_op_->tool_name,
        pending_dialog_op_->arguments);
    pending_dialog_op_.reset();
}

// Timeout check for pending destructive operations
mcp_handler_.check_pending_timeouts(kConfirmTimeoutMs);
```

在 `_exit_tree()` 中（`mcp_console_` 清理之后）添加：

```cpp
if (confirm_dialog_) {
    confirm_dialog_->queue_free();
    confirm_dialog_ = nullptr;
}
mcp_handler_.set_confirm_callback(nullptr);
mcp_handler_.reset_session_flags();
```

回调实现（在 `_exit_tree()` 之后）：

```cpp
void McpEditorPlugin::_on_confirm_allow(const Variant &id) {
    mcp_handler_.resolve_pending_op(id, true, false);
}

void McpEditorPlugin::_on_confirm_deny(const Variant &id) {
    mcp_handler_.resolve_pending_op(id, false, false);
}

void McpEditorPlugin::_on_confirm_allow_all(const Variant &id) {
    mcp_handler_.resolve_pending_op(id, true, true);
}
```

在 `_bind_methods()` 中添加：

```cpp
ClassDB::bind_method(D_METHOD("_on_confirm_allow", "id"), &McpEditorPlugin::_on_confirm_allow);
ClassDB::bind_method(D_METHOD("_on_confirm_deny", "id"), &McpEditorPlugin::_on_confirm_deny);
ClassDB::bind_method(D_METHOD("_on_confirm_allow_all", "id"), &McpEditorPlugin::_on_confirm_allow_all);
```

### 验证

1. 编译通过
2. 用 MCP Inspector 调用 `delete_node` → 弹出确认窗口
3. 点击 "Deny" → 收到 JSON-RPC 错误 `Destructive tool denied by user`
4. 再次调用 `delete_node` → 点击 "Allow" → 工具执行成功
5. 调用 `delete_node` → 点击 "Allow All This Session" → 后续 `delete_node` 直接执行不再弹窗
6. 弹窗后 30s 无操作 → 自动超时拒绝

---

## Q3: 桥接协议修复（\n → 长度前缀）

### 步骤 1: bridge.cpp — send_command 改造

**文件**: `extensions/src/runtime/bridge.cpp`

找到 `send_command()` 方法。将现有发送逻辑（构造 JSON + push_back('\n')）替换为：

```cpp
Dictionary RuntimeBridge::send_command(const String &cmd, const Dictionary &params, int timeout_ms) {
    if (status_ != CONNECTED) {
        Dictionary r;
        r["ok"] = false;
        r["error"] = "Bridge not connected";
        return r;
    }

    Dictionary msg;
    msg["cmd"] = cmd;
    msg["params"] = params;
    msg["id"] = next_id_++;

    PackedByteArray json_bytes = JSON::stringify(msg).to_utf8_buffer();

    // 4-byte big-endian length header
    const uint32_t len = static_cast<uint32_t>(json_bytes.size());
    PackedByteArray frame;
    frame.resize(4 + json_bytes.size());
    auto *p = frame.ptrw();
    p[0] = static_cast<uint8_t>((len >> 24) & 0xFF);
    p[1] = static_cast<uint8_t>((len >> 16) & 0xFF);
    p[2] = static_cast<uint8_t>((len >> 8) & 0xFF);
    p[3] = static_cast<uint8_t>(len & 0xFF);
    std::copy_n(json_bytes.ptr(), json_bytes.size(), p + 4);

    Error err = tcp_->put_data(frame);
    if (err != OK) {
        disconnect();
        Dictionary r;
        r["ok"] = false;
        r["error"] = "Send failed";
        return r;
    }

    return read_response(timeout_ms);
}
```

### 步骤 2: bridge.cpp — read_response 改造

将 `read_response()` 替换为长度前缀解析：

```cpp
Dictionary RuntimeBridge::read_response(int timeout_ms) {
    PackedByteArray header_buf;
    header_buf.resize(4);
    int header_bytes_read = 0;
    uint32_t msg_length = 0;
    bool header_done = false;
    PackedByteArray body;

    const uint64_t start = Time::get_singleton()->get_ticks_msec();

    while (true) {
        const uint64_t elapsed = Time::get_singleton()->get_ticks_msec() - start;
        if (elapsed >= static_cast<uint64_t>(timeout_ms)) break;

        tcp_->poll();
        if (tcp_->get_status() != StreamPeerTCP::STATUS_CONNECTED) {
            disconnect();
            Dictionary r;
            r["ok"] = false;
            r["error"] = "Connection lost during read";
            return r;
        }

        const int64_t avail = tcp_->get_available_bytes();
        if (avail <= 0) {
            OS::get_singleton()->delay_msec(5);
            continue;
        }

        Array chunk = tcp_->get_partial_data(static_cast<int>(avail));
        if (static_cast<Error>(static_cast<int>(chunk[0])) != OK) continue;
        PackedByteArray data = chunk[1];

        if (!header_done) {
            // Accumulate into 4-byte header
            const int needed = 4 - header_bytes_read;
            const int to_copy = MIN(needed, data.size());
            std::copy_n(data.ptr(), to_copy, header_buf.ptrw() + header_bytes_read);
            header_bytes_read += to_copy;

            if (header_bytes_read == 4) {
                msg_length = (static_cast<uint32_t>(header_buf[0]) << 24) |
                             (static_cast<uint32_t>(header_buf[1]) << 16) |
                             (static_cast<uint32_t>(header_buf[2]) << 8) |
                              static_cast<uint32_t>(header_buf[3]);
                if (msg_length == 0 || msg_length > 65536) {
                    disconnect();
                    Dictionary r;
                    r["ok"] = false;
                    r["error"] = "Invalid message length";
                    return r;
                }
                header_done = true;
                body.reserve(static_cast<int>(msg_length));

                // Remaining bytes after header are body data
                const int remaining = data.size() - to_copy;
                if (remaining > 0) {
                    body.append_array(/* slice data from to_copy */);
                    // Manual slice since PackedByteArray has no slice:
                    PackedByteArray tail;
                    tail.resize(remaining);
                    std::copy_n(data.ptr() + to_copy, remaining, tail.ptrw());
                    body.append_array(tail);
                }
            }
        } else {
            body.append_array(data);
        }

        if (header_done && body.size() >= static_cast<int>(msg_length)) {
            // Parse JSON from body
            String text;
            text.parse_utf8(reinterpret_cast<const char *>(body.ptr()), static_cast<int>(msg_length));
            Ref<JSON> json;
            json.instantiate();
            if (json->parse(text) == OK) {
                Variant result = json->get_data();
                if (result.get_type() == Variant::DICTIONARY) {
                    return make_response(result);
                }
            }
            Dictionary r;
            r["ok"] = false;
            r["error"] = "Invalid JSON in response";
            return r;
        }
    }

    // Timeout
    Dictionary r;
    r["ok"] = false;
    r["pending"] = true;
    return r;
}
```

### 步骤 3: game_bridge.cpp — send_response 改造

**文件**: `extensions/src/runtime/game_bridge.cpp`

找到 `send_response()` 方法，替换为：

```cpp
void GameBridgeNode::send_response(const Ref<StreamPeerTCP> &client, const Dictionary &msg) {
    if (!client.is_valid()) return;

    PackedByteArray json_bytes = JSON::stringify(msg).to_utf8_buffer();
    if (json_bytes.size() > MAX_MESSAGE_SIZE) {
        log_error("game_bridge", "Response too large");
        return;
    }

    // 4-byte big-endian length header
    const uint32_t len = static_cast<uint32_t>(json_bytes.size());
    PackedByteArray frame;
    frame.resize(4 + json_bytes.size());
    auto *p = frame.ptrw();
    p[0] = static_cast<uint8_t>((len >> 24) & 0xFF);
    p[1] = static_cast<uint8_t>((len >> 16) & 0xFF);
    p[2] = static_cast<uint8_t>((len >> 8) & 0xFF);
    p[3] = static_cast<uint8_t>(len & 0xFF);
    std::copy_n(json_bytes.ptr(), json_bytes.size(), p + 4);

    client->put_data(frame);
}
```

### 步骤 4: game_bridge.cpp — read_clients 改造

将 `read_clients()` 中的 `\n` 分割逻辑替换为长度前缀解析。核心逻辑：

```cpp
void GameBridgeNode::read_clients() {
    if (!client_.is_valid()) return;

    // Read available bytes into read_buf_
    client_->poll();
    if (client_->get_status() != StreamPeerTCP::STATUS_CONNECTED) {
        client_.unref();
        reset_read_state_internal();
        return;
    }

    const int64_t avail = client_->get_available_bytes();
    if (avail > 0) {
        Array chunk = client_->get_partial_data(static_cast<int>(avail));
        if (static_cast<Error>(static_cast<int>(chunk[0])) == OK) {
            PackedByteArray data = chunk[1];
            read_buf_.append_array(data);

            // Buffer limit check
            if (read_buf_.size() > BUFFER_LIMIT) {
                log_warn("game_bridge", "Buffer overflow, resetting");
                reset_read_state_internal();
                return;
            }
        }
    }

    // Parse length-prefixed messages from read_buf_
    while (true) {
        // Need at least 4 bytes for header
        if (read_offset_ + 4 > read_buf_.size()) break;

        const uint8_t *p = read_buf_.ptr() + read_offset_;
        const uint32_t msg_len = (static_cast<uint32_t>(p[0]) << 24) |
                                 (static_cast<uint32_t>(p[1]) << 16) |
                                 (static_cast<uint32_t>(p[2]) << 8) |
                                  static_cast<uint32_t>(p[3]);

        if (msg_len == 0 || msg_len > MAX_MESSAGE_SIZE) {
            log_warn("game_bridge", "Invalid message length, resetting buffer");
            reset_read_state_internal();
            return;
        }

        // Check if we have the full message
        if (read_offset_ + 4 + static_cast<int64_t>(msg_len) > read_buf_.size()) break;

        read_offset_ += 4;

        // Parse JSON from body
        String text;
        text.parse_utf8(reinterpret_cast<const char *>(read_buf_.ptr() + read_offset_),
                        static_cast<int>(msg_len));
        read_offset_ += static_cast<int64_t>(msg_len);

        Ref<JSON> json;
        json.instantiate();
        if (json->parse(text) != OK) {
            log_warn("game_bridge", "Invalid JSON message");
            continue;
        }

        Variant msg = json->get_data();
        if (msg.get_type() != Variant::DICTIONARY) {
            log_warn("game_bridge", "Message is not a dictionary");
            continue;
        }

        Dictionary msg_dict = msg;
        String cmd = msg_dict.get("cmd", "");
        Dictionary params = msg_dict.get("params", Dictionary());
        Variant id = msg_dict.get("id", Variant());

        Dictionary result = dispatch(cmd, params);
        result["id"] = id;
        send_response(client_, result);
    }

    // Trim processed bytes
    if (read_offset_ > 0) {
        const int remaining = read_buf_.size() - static_cast<int>(read_offset_);
        if (remaining > 0) {
            PackedByteArray new_buf;
            new_buf.resize(remaining);
            std::copy_n(read_buf_.ptr() + read_offset_, remaining, new_buf.ptrw());
            read_buf_ = new_buf;
        } else {
            read_buf_.clear();
        }
        read_offset_ = 0;
    }
}
```

### 步骤 5: game_bridge.hpp 清理

**文件**: `extensions/src/runtime/game_bridge.hpp`

移除不再需要的成员变量：

```cpp
// 删除这些行:
godot::String read_text_;
int utf8_retries_ = 0;
int64_t utf8_fail_offset_ = -1;
int64_t consumed_bytes_ = 0;
```

保留：

```cpp
godot::PackedByteArray read_buf_;
int64_t read_offset_ = 0;
```

简化 `reset_read_state_internal()`:

```cpp
void GameBridgeNode::reset_read_state_internal() {
    read_buf_.clear();
    read_offset_ = 0;
}
```

### 验证

1. 编译通过
2. 启动 Godot 编辑器 + 运行游戏 → 桥接连接成功
3. 通过桥接调用 `get_scene_tree` → 返回正确结果
4. 游戏中 `print("line1\nline2")` 不会导致桥接消息错位（Sprint 3 WebSocket 后完全解决，此处仅修复协议层）

---

## L3: 多编辑器端口回退

### 步骤 1: McpEditorPlugin 成员

**文件**: `extensions/src/editor_plugin.hpp`

在 `McpEditorPlugin` 类的 `private` 区域添加：

```cpp
int actual_http_port_ = 9600;
int actual_bridge_port_ = 9601;
```

在 `public` 区域添加：

```cpp
int actual_http_port() const noexcept { return actual_http_port_; }
int actual_bridge_port() const noexcept { return actual_bridge_port_; }
```

### 步骤 2: _enter_tree 端口回退

**文件**: `extensions/src/editor_plugin.cpp`

将 `_enter_tree()` 中的单次 `http_server_.start()` 调用（约第 109 行）替换为回退循环：

```cpp
    // Port fallback: try http_port_ + 0..9
    constexpr int kMaxPortAttempts = 10;
    bool server_started = false;
    int port_offset = 0;

    for (int i = 0; i < kMaxPortAttempts; ++i) {
        const int try_port = http_port_ + i;
        const Error err = http_server_.start(
            static_cast<uint16_t>(try_port), &mcp_handler_, http_host_);
        if (err == OK) {
            actual_http_port_ = try_port;
            actual_bridge_port_ = bridge_port_ + i;
            port_offset = i;
            server_started = true;
            break;
        }
        log_warn("plugin",
            String("Port ") + String::num_int64(try_port) +
            String(" in use, trying next..."));
    }

    if (!server_started) {
        log_error("plugin",
            String("Failed to start HTTP server after ") +
            String::num_int64(kMaxPortAttempts) + String(" attempts"));
        return;
    }
```

更新后续使用端口的地方：

```cpp
    runtime_bridge_.set_port(actual_bridge_port_);
    // ...
    http_server_.set_test_engine(&test_engine_);
    // ...
    log_info("plugin", String("Godot MCP v") + String(GODOT_MCP_PLUGIN_VERSION) +
                           String(" ready on HTTP :") + String::num_int64(actual_http_port_) +
                           String(" (") + String::num_int64(registry_.builtin_tool_count()) +
                           String(" builtin tools)"));
```

### 步骤 3: restart_server 同样改造

**文件**: `extensions/src/editor_plugin.cpp:84-93`

将 `restart_server()` 中的单次 start 替换为同样的回退循环：

```cpp
void McpEditorPlugin::restart_server() {
    http_server_.stop();
    runtime_bridge_.disconnect();
    load_config();

    constexpr int kMaxPortAttempts = 10;
    bool server_started = false;

    for (int i = 0; i < kMaxPortAttempts; ++i) {
        const int try_port = http_port_ + i;
        const Error err = http_server_.start(
            static_cast<uint16_t>(try_port), &mcp_handler_, http_host_);
        if (err == OK) {
            actual_http_port_ = try_port;
            actual_bridge_port_ = bridge_port_ + i;
            server_started = true;
            break;
        }
    }

    if (!server_started) {
        log_error("plugin", "Server restart failed — all ports in use");
        started_ = false;
        return;
    }

    http_server_.set_test_engine(&test_engine_);
    runtime_bridge_.set_port(actual_bridge_port_);
    started_ = true;
    log_info("plugin", String("Server restarted on HTTP :") + String::num_int64(actual_http_port_));
}
```

### 步骤 4: Dock 显示实际端口

**文件**: `extensions/src/ui/mcp_dock.cpp`

在 `update_status()` 中（`plugin_->is_started()` 为 true 的分支），修改状态文本：

```cpp
    if (plugin_ && plugin_->is_started()) {
        const int cfg_port = plugin_->http_port();
        const int actual_port = plugin_->actual_http_port();

        String st;
        if (actual_port != cfg_port) {
            st = String("[ON :") + String::num_int64(actual_port) +
                 String("] (cfg ") + String::num_int64(cfg_port) +
                 String(" in use)");
        } else {
            st = String("[ON :") + String::num_int64(actual_port) + String("]");
        }
        // ... set status_icon_ text and green color ...
    }
```

### 验证

1. 编译通过
2. 打开编辑器 A → 绑定 `:9600`，状态显示 `[ON :9600]`
3. 打开编辑器 B（同项目）→ 回退到 `:9601`，状态显示 `[ON :9601] (cfg 9600 in use)`
4. 关闭编辑器 A → 重启编辑器 B → 回到 `:9600`
