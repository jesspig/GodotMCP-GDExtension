# Sprint 2 开发文档

> 精确到代码级的实现指南。每个步骤包含：修改位置（文件:行号）、完整代码、集成点、验证方法。

---

## M3': McpConsole 增强（搜索 + Revert）

### 步骤 1: LogEntry 新增字段

**文件**: `extensions/src/ui/mcp_logger.hpp:14-21`

在 `LogEntry` 结构体中（`duration_ms` 之后）添加：

```cpp
struct LogEntry {
    godot::String timestamp;
    godot::String tool_name;
    bool success = false;
    godot::Dictionary args;
    godot::Dictionary result;
    double duration_ms = 0.0;
    godot::String operation_id;       // ← 新增
    bool supports_undo = false;       // ← 新增
};
```

### 步骤 2: McpConsole 新增搜索栏和 Revert 列

**文件**: `extensions/src/ui/mcp_console.hpp`

在 `McpConsole` 类的 `private` 区域添加：

```cpp
godot::LineEdit *search_bar_ = nullptr;
godot::String current_filter_;

void _on_search_text_changed(const godot::String &text);
void _on_revert_button_pressed();
```

**文件**: `extensions/src/ui/mcp_console.cpp`

#### 2a. 构造函数 — 添加搜索栏

在工具栏创建代码之后（`auto_scroll_chk_` 创建之后），添加搜索栏：

```cpp
    // Search bar (after auto_scroll_chk_)
    HBoxContainer *search_row = memnew(HBoxContainer);
    content->add_child(search_row);  // 注意：content 在 VSplit 之前

    Label *search_label = memnew(Label);
    search_label->set_text("Filter:");
    search_row->add_child(search_label);

    search_bar_ = memnew(LineEdit);
    search_bar_->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    search_bar_->set_placeholder("Search by tool name...");
    search_row->add_child(search_bar_);
```

#### 2b. setup_tree_columns — 添加第 3 列

将现有 2 列改为 3 列：

```cpp
void McpConsole::setup_tree_columns() {
    log_tree_->set_columns(3);  // 从 2 改为 3
    log_tree_->set_column_titles_visible(false);

    log_tree_->set_column_expand(0, false);  // Time
    log_tree_->set_column_expand(1, true);   // Tool
    log_tree_->set_column_expand(2, false);  // Action

    log_tree_->set_column_custom_minimum_width(0, 170);  // Time
    log_tree_->set_column_custom_minimum_width(1, 250);  // Tool
    log_tree_->set_column_custom_minimum_width(2, 80);   // Action
}
```

#### 2c. add_tree_entry — 添加 Revert 按钮文本

在现有 `add_tree_entry()` 中（`item->set_custom_color(1, tint)` 之后）添加：

```cpp
    // Action column: show "Revert" for undoable operations
    if (entry.supports_undo) {
        item->set_text(2, "Revert");
        item->set_custom_color(2, Color(0.3f, 0.7f, 1.0f));
        item->set_metadata(2, index);  // Store index for click handler
    }
```

#### 2d. 搜索过滤逻辑

在信号连接区域添加：

```cpp
search_bar_->connect("text_changed", Callable(this, "_on_search_text_changed"));
```

在 `_bind_methods()` 中添加：

```cpp
ClassDB::bind_method(D_METHOD("_on_search_text_changed", "text"),
    &McpConsole::_on_search_text_changed);
```

实现：

```cpp
void McpConsole::_on_search_text_changed(const String &text) {
    current_filter_ = text.to_lower();
    rebuild_log();  // Rebuild with filter applied
}
```

修改 `rebuild_log()` 和 `on_log_appended()` 添加过滤逻辑：

```cpp
void McpConsole::rebuild_log() {
    log_tree_->clear();
    for (size_t i = 0; i < logged_entries_.size(); i++) {
        const LogEntry &entry = logged_entries_[i];
        // Apply filter
        if (!current_filter_.is_empty() &&
            entry.tool_name.to_lower().find(current_filter_) < 0) {
            continue;
        }
        add_tree_entry(entry, static_cast<int>(i));
    }
    update_toolbar_state();
    update_detail();
}

void McpConsole::on_log_appended(const LogEntry &entry) {
    logged_entries_.push_back(entry);
    while (logged_entries_.size() > kMaxVisible) {
        logged_entries_.pop_front();
        // Note: Tree rebuild needed since indices shifted
    }

    // Apply filter before adding
    if (!current_filter_.is_empty() &&
        entry.tool_name.to_lower().find(current_filter_) < 0) {
        // Entry doesn't match filter — don't add to tree, but still store
        update_toolbar_state();
        return;
    }

    add_tree_entry(entry, static_cast<int>(logged_entries_.size()) - 1);
    update_toolbar_state();
}
```

#### 2e. Revert 点击处理

修改 `_on_item_activated()`（双击）来处理 Revert 列的点击：

```cpp
void McpConsole::_on_item_activated() {
    TreeItem *selected = log_tree_->get_selected();
    if (!selected) return;

    // Check if Revert column was activated
    Variant meta2 = selected->get_metadata(2);
    if (meta2.get_type() == Variant::INT) {
        int idx = static_cast<int>(meta2);
        if (idx >= 0 && static_cast<size_t>(idx) < logged_entries_.size()) {
            const LogEntry &entry = logged_entries_[idx];
            if (entry.supports_undo) {
                EditorUndoRedoManager *mgr =
                    EditorInterface::get_singleton()->get_editor_undo_redo();
                if (mgr && mgr->has_undo()) {
                    mgr->undo();
                    // Update the tree item to show "Undone"
                    selected->set_text(2, "Undone");
                    selected->set_custom_color(2, Color(0.5f, 0.5f, 0.5f));
                }
            }
        }
        return;
    }

    // Existing collapse/expand behavior
    if (selected->get_first_child()) {
        selected->set_collapsed(!selected->is_collapsed());
    }
}
```

需要新增 include：

```cpp
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
```

### 验证

1. 编译通过
2. MCP Console 底部出现搜索栏
3. 输入 "delete" → 仅显示 delete 相关调用
4. 清空搜索 → 恢复全部显示
5. 双击 supports_undo 条目的 "Revert" 列 → 执行 undo，显示 "Undone"

---

## L2: 操作回滚 MCP 工具

### 步骤 1: McpHandler 新增 operation_id 生成

**文件**: `extensions/src/server/mcp/mcp_handler.cpp`

在文件顶部（`using namespace godot;` 之后，`namespace godot_mcp {` 之后）添加：

```cpp
namespace {
std::atomic<uint64_t> g_op_counter{0};

String generate_operation_id() {
    const uint64_t seq = g_op_counter.fetch_add(1, std::memory_order_relaxed);
    const uint64_t now = Time::get_singleton()->get_ticks_usec();
    return String("op_") + String::num_int64(now, 16) +
           String("_") + String::num_int64(seq, 16);
}
} // anonymous namespace
```

需要新增 include：

```cpp
#include <atomic>
```

### 步骤 2: ToolCallLog 增强

**文件**: `extensions/src/server/mcp/mcp_handler.hpp:35-42`

在 `ToolCallLog` 结构体中添加字段：

```cpp
struct ToolCallLog {
    String timestamp;
    String tool_name;
    bool success;
    Dictionary args;
    Dictionary result;
    double duration_ms;
    String operation_id;       // ← 新增
    bool supports_undo;        // ← 新增
};
```

### 步骤 3: handle_tools_call 生成 operation_id

**文件**: `extensions/src/server/mcp/mcp_handler.cpp:313-351`

在 `handle_tools_call()` 中，在破坏性拦截之后、`tool_executor_.execute()` 之前生成 operation_id：

```cpp
Dictionary McpHandler::handle_tools_call(const Dictionary &params, const Variant &id) {
    const String tool_name = params.get("name", "");
    if (tool_name.is_empty()) {
        return make_jsonrpc_error(id, kInvalidParams, "Missing 'name' in tool call");
    }

    const Dictionary args = /* ... existing ... */;

    // Generate operation_id for all tool calls
    const String op_id = generate_operation_id();

    // Check if tool supports undo (for logging)
    bool tool_supports_undo = false;
    if (registry_) {
        tool_supports_undo = registry_->tool_supports_undo(tool_name);
    }

    // ── Destructive tool interception (Sprint 1 Q2) ──
    // ... existing code ...

    // Delegate to ToolExecutor
    Dictionary exec_result = tool_executor_.execute(tool_name, args);

    // Structured log callback
    if (log_callback_) {
        ToolCallLog log_entry;
        log_entry.timestamp = Time::get_singleton()->get_datetime_string_from_system();
        log_entry.tool_name = tool_name;
        log_entry.success = !exec_result.has("_exec_error");
        log_entry.args = args;
        log_entry.result = ToolExecutor::extract_result(exec_result);
        log_entry.duration_ms = exec_result.get("_exec_duration_ms", 0.0);
        log_entry.operation_id = op_id;
        log_entry.supports_undo = tool_supports_undo;
        log_callback_(log_entry);
    }

    // ... rest of existing code ...
}
```

### 步骤 4: HandlerRegistry 新增 tool_supports_undo

**文件**: `extensions/src/server/registry/handler_registry.hpp`

在 `public` 区域添加：

```cpp
[[nodiscard]] bool tool_supports_undo(const godot::String &name) const;
```

**文件**: `extensions/src/server/registry/handler_registry.cpp`

实现：

```cpp
bool HandlerRegistry::tool_supports_undo(const String &name) const {
    const ITool *tool = find_itool(name);
    return tool ? tool->supports_undo() : false;
}
```

### 步骤 5: UndoRedo Action 命名注入 operation_id

**文件**: `extensions/src/server/registry/handler_registry.cpp:70-110`

在 `execute()` 方法中，`create_action` 调用处修改 action 名称格式：

```cpp
Dictionary HandlerRegistry::execute(const String &name, const Dictionary &args) {
    record_tool_call(name);

    auto info_it = tool_info_.find(name);
    bool undoable = false;
    if (info_it != tool_info_.end()) {
        const ITool *tp = find_itool(name);
        if (tp) undoable = tp->supports_undo();
    }

    auto it = itool_table_.find(name);
    if (it != itool_table_.end()) {
        if (undoable) {
            EditorUndoRedoManager *undo_redo = get_undo_redo();
            if (undo_redo) {
                // Extract operation_id from args (injected by McpHandler)
                String op_id;
                if (args.has("_operation_id")) {
                    op_id = args["_operation_id"];
                }

                // Action name includes operation_id for precise rollback
                String action_name = String("MCP: ") + name;
                if (!op_id.is_empty()) {
                    action_name += String(" [") + op_id + String("]");
                }

                undo_redo->create_action(action_name, UndoRedo::MERGE_ENDS);
                Dictionary result = it->second->execute(args);
                if (result.has("success") && result["success"].operator bool()) {
                    undo_redo->commit_action(false);
                } else {
                    undo_redo->commit_action(true);
                }
                return result;
            }
        }
        return it->second->execute(args);
    }

    Dictionary error;
    error["error"] = String("Tool not found: ") + name;
    return error;
}
```

**注意**：需要在 `handle_tools_call()` 中将 `op_id` 注入 args：

```cpp
    // Before calling tool_executor_.execute()
    Dictionary enriched_args = args.duplicate();
    enriched_args["_operation_id"] = op_id;
    Dictionary exec_result = tool_executor_.execute(tool_name, enriched_args);
```

### 步骤 6: 创建 RollbackOperationTool

**新文件**: `extensions/src/built_in/tools/meta/rollback_operation.hpp`

```cpp
#pragma once

#include "built_in/tool_base.hpp"

namespace godot_mcp {

class RollbackOperationTool : public ITool {
public:
    RollbackOperationTool() = default;
    ~RollbackOperationTool() override = default;

    godot::String name() const noexcept override { return "rollback_operation"; }
    godot::String brief() const noexcept override {
        return "Undo a specific previous AI operation by operation_id";
    }
    godot::String description() const noexcept override {
        return "Undoes the changes made by a previous tool call, identified by its "
               "operation_id. Only works for operations that support undo. "
               "The operation_id is returned in every tool call response.";
    }
    godot::String category() const noexcept override { return "meta_tools"; }
    bool is_meta() const noexcept override { return true; }

    godot::Dictionary build_input_schema() const override {
        using namespace godot;
        Dictionary props;
        {
            Dictionary op_id;
            op_id["type"] = "string";
            op_id["description"] = "The operation_id returned by the tool call to undo";
            props["operation_id"] = op_id;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        Array required;
        required.push_back("operation_id");
        s["required"] = required;
        return s;
    }

protected:
    godot::Dictionary execute_impl(const ToolContext &ctx) override;
};

} // namespace godot_mcp
```

**新文件**: `extensions/src/built_in/tools/meta/rollback_operation.cpp`

```cpp
#include "rollback_operation.hpp"
#include "built_in/cmd_utils.hpp"
#include "server/registry/handler_registry.hpp"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/classes/undo_redo.hpp>

using namespace godot;

namespace godot_mcp {

Dictionary RollbackOperationTool::execute_impl(const ToolContext &ctx) {
    const String target_op_id = args_string(ctx.args, "operation_id");
    if (target_op_id.is_empty()) {
        return ToolResult::err("INVALID_PARAMS", "operation_id is required");
    }

    EditorUndoRedoManager *mgr = EditorInterface::get_singleton()->get_editor_undo_redo();
    if (!mgr) {
        return ToolResult::err("NO_UNDO_REDO", "EditorUndoRedoManager not available");
    }

    if (!mgr->has_undo()) {
        return ToolResult::err("NOTHING_TO_UNDO", "No undo history available");
    }

    // Get the global history's underlying UndoRedo instance
    UndoRedo *undo_redo = mgr->get_history_undo_redo(
        EditorUndoRedoManager::GLOBAL_HISTORY);
    if (!undo_redo) {
        return ToolResult::err("NO_HISTORY", "Cannot access undo history");
    }

    // Search for action containing [operation_id]
    const String target_prefix = String("[") + target_op_id + String("]");

    constexpr int kMaxSteps = 100;
    for (int i = 0; i < kMaxSteps; ++i) {
        if (!undo_redo->has_undo()) {
            return ToolResult::err("NOT_FOUND",
                String("Operation not found: ") + target_op_id);
        }

        const String current_name = undo_redo->get_current_action_name();
        undo_redo->undo();

        if (current_name.find(target_prefix) >= 0) {
            // Found and undone
            Dictionary data;
            data["status"] = "undone";
            data["operation_id"] = target_op_id;
            data["action_name"] = current_name;
            return ToolResult::ok(data);
        }
    }

    return ToolResult::err("NOT_FOUND",
        String("Exceeded max undo steps (") + String::num_int64(kMaxSteps) +
        String(") without finding operation: ") + target_op_id);
}

} // namespace godot_mcp
```

### 步骤 7: 注册工具

**文件**: `extensions/src/built_in/tools/register/register_meta.hpp`

在末尾添加：

```cpp
GODOT_MCP_TOOL(RollbackOperationTool, true)  // destructive: true
```

**文件**: `extensions/src/built_in/register_itools.cpp`

在 Meta tools include 区域添加：

```cpp
#include "built_in/tools/meta/rollback_operation.hpp"
```

### 步骤 8: McpLogger LogEntry 转发 operation_id

**文件**: `extensions/src/editor_plugin.cpp`

在 `_enter_tree()` 的 `mcp_handler_.set_log_callback` lambda 中，转发新字段：

```cpp
    mcp_handler_.set_log_callback([this](const McpHandler::ToolCallLog &log) {
        McpLogger::LogEntry entry;
        entry.timestamp = log.timestamp;
        entry.tool_name = log.tool_name;
        entry.success = log.success;
        entry.args = log.args;
        entry.result = log.result;
        entry.duration_ms = log.duration_ms;
        entry.operation_id = log.operation_id;     // ← 新增
        entry.supports_undo = log.supports_undo;   // ← 新增
        logger_.append(entry);
    });
```

### 验证

1. 编译通过
2. 调用 `add_node` → 返回结果中包含 `operation_id`（如 `op_1a2b3c_0`）
3. 调用 `rollback_operation(operation_id="op_1a2b3c_0")` → 返回 `{status: "undone", operation_id: "op_1a2b3c_0"}`
4. MCP Console 中该条目显示 "Revert" 按钮
5. 调用 `rollback_operation(operation_id="nonexistent")` → 返回错误 `NOT_FOUND`

---

## L1: AssetLib 发布流水线

### 步骤 1: GitHub Actions 工作流

**新文件**: `.github/workflows/release.yml`

```yaml
name: Release

on:
  push:
    tags: ["v*"]

concurrency:
  group: release-${{ github.ref }}
  cancel-in-progress: false

jobs:
  build:
    strategy:
      fail-fast: false
      matrix:
        include:
          - os: windows-2022
            preset: win-clang-cl
            plat: windows
            lib: godot_mcp_gdext.dll
          - os: ubuntu-latest
            preset: linux-gcc
            plat: linux
            lib: libgodot_mcp_gdext.so
          - os: macos-latest
            preset: macos-clang
            plat: macos
            lib: libgodot_mcp_gdext.dylib

    runs-on: ${{ matrix.os }}
    timeout-minutes: 30

    steps:
      - uses: actions/checkout@v4

      - name: Install Ninja
        uses: seanmiddleditch/gha-setup-ninja@v5

      - name: Setup sccache
        uses: mozilla-actions/sccache-action@v0.0.8

      - name: Setup MSVC (Windows)
        if: runner.os == 'Windows'
        uses: ilammy/msvc-dev-cmd@v1

      - name: Cache dependencies
        uses: actions/cache@v4
        with:
          path: build/_deps
          key: gdext-deps-${{ runner.os }}-${{ hashFiles('extensions/CMakeLists.txt') }}

      - name: Build
        run: python main.py build --release

      - name: Upload artifact
        uses: actions/upload-artifact@v4
        with:
          name: gdext-${{ matrix.plat }}
          path: example/addons/godot_mcp/bin/${{ matrix.lib }}

  package:
    needs: build
    runs-on: ubuntu-latest
    timeout-minutes: 10
    permissions:
      contents: write

    steps:
      - uses: actions/checkout@v4

      - name: Download all artifacts
        uses: actions/download-artifact@v4
        with:
          pattern: gdext-*
          path: dist

      - name: Copy libraries into addon
        run: |
          mkdir -p example/addons/godot_mcp/bin
          cp dist/gdext-windows/*.dll example/addons/godot_mcp/bin/
          cp dist/gdext-linux/*.so example/addons/godot_mcp/bin/
          cp dist/gdext-macos/*.dylib example/addons/godot_mcp/bin/

      - name: Generate addon configs
        run: python main.py package

      - name: Rename zip with version
        id: version
        run: |
          VERSION=$(python -c "import re; print(re.search(r'PROJECT_VERSION\s+\"(.+?)\"', open('CMakeLists.txt').read()).group(1))")
          mv addons.zip "godot_mcp_v${VERSION}.zip"
          echo "VERSION=${VERSION}" >> $GITHUB_OUTPUT

      - name: Create GitHub Release
        uses: softprops/action-gh-release@v2
        with:
          files: godot_mcp_v${{ steps.version.outputs.VERSION }}.zip
          generate_release_notes: true
```

### 步骤 2: main.py 版本感知打包

**文件**: `main.py`

在 `_cmd_package()` 函数中，修改输出文件名逻辑。读取当前实现后，在打包步骤末尾将 `addons.zip` 重命名为包含版本号的文件名。

### 验证

1. `git tag v0.2.2 && git push origin v0.2.2` → 触发 CI
2. CI 矩阵构建 3 平台 → 每个产出 .dll/.so/.dylib
3. package job → 下载所有 artifact → 组装 addons 目录 → 生成 zip
4. GitHub Release 自动创建 → 包含 `godot_mcp_v0.2.2.zip`
5. 下载 zip → 解压到 Godot 项目 → 启用插件 → MCP 正常工作
