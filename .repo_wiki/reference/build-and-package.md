# 构建与打包

## 构建系统

构建系统是 **CMake**（C++ GDExtension → godot-cpp 10.0.0-rc1 通过 FetchContent）+ **Cython --embed**（Python 服务器编译）。提供了轻量 `build.py` 包装。

```bash
py -3 build.py                        # debug 构建 + addons.zip
py -3 build.py --release              # release 构建 + addons.zip
py -3 build.py --clean                # 清空 addons/bin/ + FetchContent 缓存
py -3 build.py --no-zip               # 跳过 addons.zip（快速迭代）
py -3 build.py --no-server            # 只重建 gdext dll（编辑器中快速迭代）
```

CMake 自动处理：
- 终止已运行的 `godot-mcp-server.exe`（`taskkill`/`pkill`）
- `FetchContent` 拉取 `godot-cpp 10.0.0-rc1`
- 生成 `plugin.cfg` 和 `godot_mcp.gdextension`
- 复制 dll/dylib/so 到 `example/addons/godot_mcp/bin/`
- CPack 打包 → `addons.zip`

## Python 服务器构建流程

服务器（Python/Cython）构建由 CMake 在 `CMakeLists.txt` 中处理：

1. **Cython `--embed`** 编译 `server/entry.py` → `build/entry.c`
2. **Patch PYTHONHOME**: `tools/patch_entry_c.py` 将 Python home 路径嵌入 C 文件
3. **C 编译**: 用系统 C 编译器编译 `entry_patched.c` → `build/godot-mcp-server.exe`
4. **复制 DLL**: 复制 `python3xy.dll` 到 exe 同目录

需要 `server/.venv`（含 Cython 包）。

## C++ GDExtension 构建流程

CMake 通过 `add_subdirectory(extensions/gdext)` 构建 godot-cpp + gdext 源文件：

1. FetchContent 拉取 `godot-cpp 10.0.0-rc1`
2. 添加 `/extensions/gdext/src/` 下的所有源文件
3. 链接 godot-cpp 静态库 → `godot_mcp_gdext.dll`
4. 后处理：复制到 `example/addons/godot_mcp/bin/`

## 手动构建（跳过 build.py）

```bash
cmake -B build -S .                          # 配置
cmake --build build --config Debug           # 构建
cmake --build build --config Debug --target package  # 打包
```

## CI 门禁

`.github/workflows/ci.yml`（在 Ubuntu 上运行）：

```bash
cmake -B build -S .                           # 1. CMake 配置
cmake --build build --config Debug            # 2. 构建 gdext + server
cargo test --workspace                        # 3. Rust 测试（core + 遗留 gdext，离线无 Godot）
```

CI 只在 `master` 分支的 push 和 PR 上触发。

## Release 构建

`.github/workflows/release.yml` 在 tag `v*` 推送时触发：

- 矩阵构建：ubuntu / macOS / Windows
- 使用 `cmake --build --config Release`
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

`example/addons/godot_mcp/bin/*` 在 `.gitignore` 中（`.gitkeep` 除外）。**永远不要将构建产物检入版本控制。**

## 版本管理

- 单版本源在 `CMakeLists.txt`：`set(PROJECT_VERSION "0.1.5-dev.1")`
- CMake 生成 `plugin.cfg` 时自动填充此版本号（不再使用 Cargo.toml）
- 升级 CMake 版本即可；不需要手动编辑 `plugin.cfg` 或 `Cargo.toml`

## 依赖锁定

- `godot-cpp 10.0.0-rc1`：通过 FetchContent 固定标签
- `Cargo.lock` 已提交（用于 `cargo test --workspace`）；不要随意重新生成
