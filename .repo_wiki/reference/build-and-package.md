# 构建与打包

## 构建系统

构建系统是 **CMake**（C++ GDExtension → godot-cpp 10.0.0-rc1 通过 FetchContent）。提供了轻量 `build.py` 包装。

```bash
py -3 build.py                        # debug 构建 + addons.zip
py -3 build.py --release              # release 构建 + addons.zip
py -3 build.py --clean                # 清空 CMake 缓存（保留 _deps/godot-cpp）
py -3 build.py --no-zip               # 跳过 addons.zip（快速迭代）
```

CMake 自动处理：
- `FetchContent` 拉取 `godot-cpp 10.0.0-rc1`
- 生成 `plugin.cfg` 和 `godot_mcp.gdextension`
- 复制 dll/dylib/so 到 `example/addons/godot_mcp/bin/`
- CPack 打包 → `addons.zip`

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

- `python`/`python3` 是 Microsoft Store 存根，会静默挂起——务必使用 **`py -3`**

## GDExtension 文件锁

| 文件 | 被谁锁定 | 如何处理 |
|------|---------|----------|
| `godot_mcp_gdext.dll` | Godot 编辑器（插件已加载） | 关闭编辑器或禁用插件后重建 |

## 构建产物的 Git 忽略

`example/addons/godot_mcp/bin/*` 在 `.gitignore` 中。**永远不要将构建产物检入版本控制。**

## 版本管理

- 单版本源在 `CMakeLists.txt`：`set(PROJECT_VERSION "0.1.5-dev3")`
- CMake 生成 `plugin.cfg` 时自动填充此版本号
- 升级 CMake 版本即可；不需要手动编辑 `plugin.cfg`
- `pyproject.toml` 中的 `version` 需手动同步（仅保留构建工具依赖）

## 依赖锁定

- `godot-cpp 10.0.0-rc1`：通过 FetchContent 固定标签
- Python 依赖：`uv.lock` 锁定
