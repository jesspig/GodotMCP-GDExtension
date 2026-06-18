# 构建与打包

## 构建系统

构建系统是 **CMake**（C++ GDExtension → godot-cpp 10.0.0-rc1 通过 FetchContent）。提供了轻量 `main.py` 包装。

```bash
uv run python main.py build                 # debug 构建
uv run python main.py build --release       # release 构建
uv run python main.py build --zip           # 构建 + addons.zip
uv run python main.py build --clean-cache   # 清空构建缓存
uv run python main.py build --clean         # 同 --clean-cache
uv run python main.py build -j 16           # 并行 16 作业
uv run python main.py package               # 打包 addons.zip
uv run python main.py test                  # 运行测试流水线
```

CMake 自动处理：

- `FetchContent` 拉取 `godot-cpp 10.0.0-rc1` + `ryml v0.7.0`（header-only）
- 生成 `plugin.cfg` 和 `godot_mcp.gdextension`
- 复制 dll/dylib/so 到 `example/addons/godot_mcp/bin/`
- CPack 打包 → `addons.zip`
- 自动检测 sccache/ccache、Unity build、lld-link 加速编译

## C++ GDExtension 构建流程

CMake 通过 `add_subdirectory(extensions)` 构建 godot-cpp + 扩展源文件：

1. FetchContent 拉取 `godot-cpp 10.0.0-rc1`
2. 添加 `/extensions/src/` 下的所有源文件
3. 链接 godot-cpp 静态库 → `godot_mcp_gdext.dll`
4. 后处理：复制到 `example/addons/godot_mcp/bin/`

## 手动构建（跳过 main.py）

```bash
cmake -B build -S .                          # 配置
cmake --build build --config Debug           # 构建
cmake --build build --config Debug --target package  # 打包
```

## 构建产物的 Git 忽略

`example/addons/godot_mcp/bin/*` 在 `.gitignore` 中。**永远不要将构建产物检入版本控制。**

## 版本管理

- 单版本源在 `CMakeLists.txt`：`set(PROJECT_VERSION "0.2.1")`
- CMake 生成 `plugin.cfg` 时自动填充此版本号
- 升级 CMake 版本即可；不需要手动编辑 `plugin.cfg`

## 依赖锁定

- `godot-cpp 10.0.0-rc1`：通过 FetchContent 固定标签
- `ryml v0.7.0`：header-only YAML 库
- Python 依赖：`uv.lock` 锁定
- **始终用 `uv run python`** 运行 main.py（uv 自动激活 `.venv`）
