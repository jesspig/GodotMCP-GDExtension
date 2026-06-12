# 构建与打包

## 构建系统

构建系统是 **CMake**（C++ GDExtension → godot-cpp 10.0.0-rc1 通过 FetchContent）。提供了轻量 `build.py` 包装。

```bash
uv run python build.py                # debug 构建 + addons.zip
uv run python build.py --release      # release 构建 + addons.zip
uv run python build.py --clean        # 清空 build/（保留 _deps/ FetchContent）
uv run python build.py --clean-all    # 完全清除 build/（含 _deps/）
uv run python build.py --no-zip       # 跳过 addons.zip（快速迭代）
uv run python build.py --purge-cache  # 仅清 _deps/（强制重下载）
uv run python build.py -j N           # 指定并行编译作业数（默认 = CPU 核心数）
cmake --build build --target deep-clean  # 仅清 example/addons/godot_mcp/bin/ + _deps/
```

CMake 自动处理：
- `FetchContent` 拉取 `godot-cpp 10.0.0-rc1` 和 `ryml v0.7.0`
- 生成 `plugin.cfg` 和 `godot_mcp.gdextension`（根 `CMakeLists.txt:59-83`）
- 复制 DLL/SO/DYLIB 到 `example/addons/godot_mcp/bin/`（`copy-gdext` target）
- CPack 打包 → `addons.zip`

## 构建优化（`extensions/CMakeLists.txt`）

| 优化 | 状态 | 说明 |
|------|:----:|------|
| **sccache/ccache** | 自动检测 | 根 `CMakeLists.txt:29-35`，加速增量构建 2-5x |
| **Unity (jumbo) build** | 默认 ON | batch size 自动匹配 CPU 核数（上限 32） |
| **lld-link** | 自动检测 | MSVC + lld-link 加速链接 |
| **PCH** | 已移除 | ADR-013：Unity Build 已覆盖其优化价值，移除以简化构建 |

### 安装方式

```bash
# sccache (推荐 — 纯 Rust, 无额外运行时)
winget install sccache
# 或: choco install sccache

# LLVM (含 lld-link)
winget install LLVM
```

## C++ GDExtension 构建流程

1. FetchContent 拉取 `godot-cpp 10.0.0-rc1` + `rapidyaml v0.7.0`
2. CMake GLOB 收集 `built_in/tools/*.cpp`（如有）
3. 编译所有源文件（含 X-macro 注册）→ `godot_mcp_gdext.{dll,so,dylib}`
4. 复制到 `example/addons/godot_mcp/bin/`

## 手动构建（跳过 build.py）

```bash
cmake -B build -S .                          # 配置
cmake --build build --config Debug           # 构建
cmake --build build --config Debug --target package  # 打包
cmake --build build --target deep-clean      # 仅清 addons/bin/ + _deps/
```

## CI 门禁

`.github/workflows/ci.yml`（在 Ubuntu 上运行）：

```bash
cmake -B build -S .                           # 1. CMake 配置
cmake --build build --config Debug            # 2. 构建 gdext
```

CI 只在 `master` 分支的 push 和 PR 上触发。

## Release 构建

`.github/workflows/release.yml` 在 tag `v*` 推送时触发：
- 矩阵构建：ubuntu / macOS / Windows
- 使用 `cmake --build --config Release`
- 分别上传 GDExtension 库到 Release artifacts
- 在 Ubuntu 上组装跨平台 `addons.zip`（包含三个平台的 dll/so/dylib）

### Windows 注意事项

- `build.py` 自动检测 `vswhere` + Ninja + MSVC `cl.exe`
- 陈旧缓存自动重试：检测 MSB4019/VCTargetsPath/编译器路径变更等错误模式，自动 `--clean` 后重试
- SSL 错误自动降级：`CMAKE_TLS_VERIFY=0` 重试
- **所有平台使用 `uv run python`**（裸 `py -3` 在 Microsoft Store stub 下会挂）
- **Python >=3.14**：`.python-version` 锁定 `3.14`

## GDExtension 文件锁

| 文件 | 被谁锁定 | 如何处理 |
|------|---------|----------|
| `example/addons/godot_mcp/bin/godot_mcp_gdext.dll` | Godot 编辑器（插件已加载） | 关闭编辑器或禁用插件后重建 |

## 版本管理

- 单版本源在根 `CMakeLists.txt:22`：`set(PROJECT_VERSION "0.2.1-dev1")`
- `plugin.cfg` 和 `godot_mcp.gdextension` 由 CMake 从 `PROJECT_VERSION` 自动生成（`CMakeLists.txt:59-83`）
- 升级 CMake 版本即可；不需要手动编辑 `plugin.cfg`
- `pyproject.toml` 中的 `version` 需手动同步

## 依赖锁定

- `godot-cpp 10.0.0-rc1`：通过 FetchContent 固定标签（`extensions/CMakeLists.txt:17-21`）
- `ryml v0.7.0`：通过 FetchContent git tag（`extensions/CMakeLists.txt:33-42`），GIT_SUBMODULES 包含 c4core
- Python 依赖：`uv.lock` 锁定
