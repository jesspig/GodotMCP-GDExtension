# Phase 0 — P0 阻断性修复

> 预计工期：1-2 天。不修复则产品不可用/不可分发。
> ADR-016 子决策：决策 1（预编译分发策略）、决策 3（网络绑定安全策略）。

## P0-1：修复 Release 流水线

### 现状问题

`release.yml` 的 `package` job 只复制了 `bin/*.dll/.so/.dylib`，缺少 `plugin.cfg` 和 `godot_mcp.gdextension`。用户下载 GitHub Release 的 `addons.zip` 后无法直接使用。

同时存在：
- `LICENSE` 文件使用占位符（`Copyright (c) [year] [fullname]`）
- `plugin.cfg` 的 `author=""` 为空
- `.gdextension` 缺少 ARM64 条目（Apple Silicon）

### 修改清单

#### 1. `.github/workflows/release.yml` — package job 修复

**当前**（`release.yml:63-69`）：
```yaml
mkdir -p godot/addons/godot_mcp/bin
cp libs/gdext-windows-latest/godot_mcp_gdext.dll godot/addons/godot_mcp/bin/
cp libs/gdext-ubuntu-latest/libgodot_mcp_gdext.so godot/addons/godot_mcp/bin/
cp libs/gdext-macos-latest/libgodot_mcp_gdext.dylib godot/addons/godot_mcp/bin/
cd godot/addons
zip -r ../../addons.zip godot_mcp/
```

**修改后**：
```yaml
- uses: actions/checkout@v4
- name: Generate config files
  run: cmake -B build -S .
- name: Assemble addons.zip
  run: |
    mkdir -p godot/addons/godot_mcp/bin
    cp libs/gdext-windows-latest/godot_mcp_gdext.dll godot/addons/godot_mcp/bin/
    cp libs/gdext-ubuntu-latest/libgodot_mcp_gdext.so godot/addons/godot_mcp/bin/
    cp libs/gdext-macos-latest/libgodot_mcp_gdext.dylib godot/addons/godot_mcp/bin/
    cp build/example/addons/godot_mcp/plugin.cfg godot/addons/godot_mcp/
    cp build/example/addons/godot_mcp/godot_mcp.gdextension godot/addons/godot_mcp/
    cd godot/addons
    zip -r ../../addons.zip godot_mcp/
```

#### 2. `LICENSE` — 替换占位符

填写真实版权声明（MIT License）。

#### 3. `CMakeLists.txt` 根 — `plugin.cfg` 中 `author` 字段

```cmake
file(WRITE ${PLUGIN_CFG}
  "[plugin]\n"
  "name=\"Godot MCP\"\n"
  "description=\"Model Context Protocol bridge for Godot Engine.\"\n"
  "author=\"Your Name\"\n"           # 修改此行
  "version=\"${PROJECT_VERSION}\"\n"
  "script=\"\"\n")
```

#### 4. `.gdextension` — Apple Silicon 支持

在 `CMakeLists.txt` 的 `.gdextension` 生成部分增加：

```cmake
"macos.debug=\\\"res://addons/godot_mcp/bin/libgodot_mcp_gdext.dylib\\\"\\n"
"macos.release=\\\"res://addons/godot_mcp/bin/libgodot_mcp_gdext.dylib\\\"\\n"
```

> 注：Godot 4.6 的 GDExtension 对 macOS 使用通用二进制条目（不含架构后缀），universal binary 或单一架构均可。

### 验证标准

```bash
# 1. 触发 Release 构建
git tag v0.2.0-test && git push origin v0.2.0-test

# 2. 下载 addons.zip
# 3. 解压到 Godot 4.6 项目的 addons/ 目录
# 4. 启动 Godot → Project Settings → Plugins → 启用 Godot MCP
# 5. curl http://127.0.0.1:9600/mcp → 返回 MCP 协议响应
```

---

## P0-2：CI 跨平台编译验证

### 现状问题

`ci.yml` 仅在 `ubuntu-latest` 上构建，PR 不会发现 Windows/macOS 编译错误。

### 修改清单

#### `.github/workflows/ci.yml` — 三平台矩阵

```yaml
strategy:
  fail-fast: false
  matrix:
    os: [ubuntu-latest, macos-latest, windows-latest]

runs-on: ${{ matrix.os }}
```

其余步骤与当前 `ci.yml` 的步骤一致（checkout → cache → cmake configure → cmake build）。

### 验证标准

PR 提交后三平台 CI 全绿。

---

## P0-3：GameBridge 绑定地址修复

### 现状问题

`game_bridge.cpp:92` 的 `server_->listen(port_)` 无绑定地址参数，Godot 的 `TCPServer` 默认绑定所有接口（`*`），游戏运行时桥接端口可能对外暴露。

### 修改清单

#### `extensions/src/runtime/game_bridge.cpp`

```cpp
// 当前（game_bridge.cpp:92）：
Error err = server_->listen(port_);

// 修改为：
Error err = server_->listen(port_, "127.0.0.1");
```

### 验证标准

```bash
# 1. 构建并启用插件
# 2. 运行游戏场景
# 3. netstat -an | grep 9601
#    应显示 127.0.0.1:9601 而非 0.0.0.0:9601
```
