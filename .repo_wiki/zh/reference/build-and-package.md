# 构建与打包

## 构建系统

构建系统是 **CMake + Corrosion**（Rust-CMake 桥接）。提供了一个轻量 `build.py` 包装。

```bash
py -3 build.py                        # debug 构建 + addons.zip
py -3 build.py --release              # release 构建 + addons.zip
py -3 build.py --clean                # cargo clean + 清空 addons/bin/
py -3 build.py --no-zip               # 跳过 addons.zip（快速迭代）
py -3 build.py --no-server            # 只重建 dll
```

CMake 自动处理：
- 终止已运行的 `godot-mcp-server.exe`（`taskkill`/`pkill`）
- 生成 `plugin.cfg` 和 `godot_mcp.gdextension`
- 复制 dll 到 `godot/addons/godot_mcp/bin/`
- CPack 打包 → `addons.zip`

## 手动构建（跳过 build.py）

```bash
cmake -B build -S .                          # 配置
cmake --build build --config Debug           # 构建
cmake --build build --config Debug --target package  # 打包
```

## CI 门禁

`.github/workflows/ci.yml`（在 Ubuntu 上运行）：

```bash
cargo fmt --check --all                       # 1. 格式化检查
cargo clippy --workspace -- -D warnings       # 2. 严格 lint
cmake -B build -S .                           # 3a. CMake 配置
cmake --build build --config Debug            # 3b. 构建 gdext + server
cargo test --workspace                        # 4. 测试（50 个，离线无 Godot）
```

CI 只在 `master` 分支的 push 和 PR 上触发。

## Release 构建

`.github/workflows/release.yml` 在 tag `v*` 推送时触发：

- 矩阵构建：ubuntu / macOS / Windows
- 使用 `cmake -DRELEASE=ON`
- 分别上传 GDExtension 库和 Server 二进制到 Release artifacts
- 在 Ubuntu 上组装跨平台 `addons.zip`（包含三个平台的 dll/so/dylib）

### Windows 注意事项

- `python`/`python3` 是 Microsoft Store 存根，会静默挂起——务必使用 **`py -3`**
- Release 构建中的 server 二进制被重命名为 `godot-mcp-server_windows.exe`

## GDExtension 文件锁

| 文件 | 被谁锁定 | 如何处理 |
|------|---------|----------|
| `godot-mcp-server.exe` | 正在运行的 MCP 客户端 | CMake `execute_process(taskkill)` 自动杀；手动构建需先 `taskkill /F /IM godot-mcp-server.exe` |
| `godot_mcp_gdext.dll` | Godot 编辑器（插件已加载） | 关闭编辑器或禁用插件后重建 |

## 构建产物的 Git 忽略

`godot/addons/godot_mcp/bin/*` 在 `.gitignore` 中（`.gitkeep` 除外）。**永远不要将构建产物检入版本控制。**

## 版本管理

- 单版本源在 `Cargo.toml [workspace.package].version`
- CMakeLists.txt 从 Cargo.toml 解析版本，生成 `plugin.cfg` 时自动填充
- 升级 cargo 版本即可；不需要手动编辑 `plugin.cfg`

## 依赖锁定

- `godot = "=0.5"` — 严格锁定
- `rmcp = "=1.7"` — 严格锁定
- `Cargo.lock` 已提交（二进制 crate）；不要随意重新生成