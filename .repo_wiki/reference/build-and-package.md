# 构建与打包

## 构建系统

构建系统是 **CMake**（C++ GDExtension → godot-cpp 10.0.0-rc1 通过 FetchContent）。提供了轻量 `main.py` 包装。

```bash
uv run python main.py build                   # debug 构建 + 复制到 example/
uv run python main.py build --release         # release 构建
uv run python main.py build --zip             # 构建后额外打包 addons.zip
uv run python main.py build --clean-cache     # 清 build/ 缓存（保留 _deps/）
uv run python main.py build --clean-deps      # ⚠ 仅清 _deps/，严重网络风险
uv run python main.py build --clean-all       # ⚠ 删整个 build/（含 _deps/），严重网络风险
uv run python main.py build --clean           # 同 --clean-cache（向后兼容）
uv run python main.py build -j N              # 指定并行编译作业数（默认 = CPU 核心数）
cmake --build build --target deep-clean       # 仅清 example/addons/godot_mcp/bin/ + _deps/
```

CMake 自动处理：

- `FetchContent` 拉取 `godot-cpp 10.0.0-rc1` 和 `ryml v0.7.0`
- 生成 `plugin.cfg` 和 `godot_mcp.gdextension`（由 `scripts/_addon.py:generate_addon_configs()` 处理）
- 复制 DLL/SO/DYLIB 到 `example/addons/godot_mcp/bin/`（`copy-gdext` target）
- CPack 打包 → `addons.zip`

## 构建优化（`extensions/CMakeLists.txt`）

| 优化 | 状态 | 说明 |
|------|:----:|------|
| **sccache/ccache** | 自动检测 | `cmake/cache.cmake`，加速增量构建 2-5x |
| **Unity (jumbo) build** | 默认 ON | batch size = min(CPU核数, ceil(源文件数/4))，无硬上限 |
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
2. CMake GLOB 收集 `built_in/tools/**/*.cpp`（如有；当前所有工具为 header-only `.hpp`，由 X-macro 编译期注册）
3. 编译所有源文件（含 X-macro 注册）→ `godot_mcp_gdext.{dll,so,dylib}`
4. 复制到 `example/addons/godot_mcp/bin/`

## 手动构建（跳过 main.py）

```bash
cmake -B build -S .                          # 配置
cmake --build build --config Debug           # 构建
cmake --build build --config Debug --target package  # 打包
cmake --build build --target deep-clean      # 仅清 addons/bin/ + _deps/
```

## Release 构建

`.github/workflows/release.yml` 在 tag `v*` 推送时触发：

- 矩阵构建：ubuntu-latest / macos-latest / windows-2022
- 使用 `cmake --build --config Release` + sccache + Ninja
- 分别上传 GDExtension 库到 Release artifacts
- 在 ubuntu-latest 上组装跨平台 `addons.zip`（包含三个平台的 dll/so/dylib）

详见 [CI/CD 流水线](ci-cd.md)。

### Windows 注意事项

- `main.py` 自动检测 `vswhere` + Ninja + MSVC `cl.exe`
- 陈旧缓存自动重试：检测 MSB4019/VCTargetsPath/编译器路径变更等错误模式，自动 `--clean-cache` 后重试
- SSL 错误自动降级：`CMAKE_TLS_VERIFY=0` 重试
- **所有平台使用 `uv run python`**（裸 `py -3` 在 Microsoft Store stub 下会挂）
- **Python >=3.14**：`.python-version` 锁定 `3.14`

## GDExtension 热重载

`.gdextension` 设 `reloadable = true`（Godot 4.2+ 官方机制，`GDExtensionManager::reload_extension()` 自动检测文件变更并重载扩展）。`main.py build` 直接覆盖 DLL，编辑器在检测到变更后自动重载。

**Windows 注意事项**：因 OS Loader 锁定 DLL，覆盖可能失败（视系统版本和配置而异），此时关闭编辑器重试。

**已知约束**：仅限编辑器构建；修改 Godot base class 后需重启编辑器。

## 版本管理

- 单版本源在根 `CMakeLists.txt:10`：`set(PROJECT_VERSION "0.2.2-dev3")`
- `plugin.cfg` 和 `godot_mcp.gdextension` 由 `main.py build` 调用 `scripts/_addon.py:generate_addon_configs()` 从 `PROJECT_VERSION` 自动生成
- 升级 CMake 版本即可；不需要手动编辑 `plugin.cfg`
- `pyproject.toml` 中的 `version` 需手动同步（CMake 使用 `0.2.2-dev3`，pyproject.toml 使用 PEP 508 `0.2.2.dev1`，为正常格式差异）

## 依赖锁定

- `godot-cpp 10.0.0-rc1`：通过 FetchContent 固定标签（`extensions/CMakeLists.txt:10-16`）
- `ryml v0.7.0`：通过 FetchContent git tag（`extensions/CMakeLists.txt:23-30`），GIT_SUBMODULES 包含 c4core
- Python 依赖：`uv.lock` 锁定
