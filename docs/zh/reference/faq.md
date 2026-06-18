# 常见问题

## 连接问题

### GodotMCP 启动后无法连接

1. 确认插件已在项目设置中启用
2. 确认端口未被占用：`curl http://localhost:9600/mcp`
3. 检查环境变量 `GODOT_MCP_HTTP_PORT` 是否设置了非默认端口
4. 关闭编辑器后重新打开

### 连接成功后部分工具不可用

工具通过 X-macro 注册机制自动收集，所有 `extensions/src/built_in/tools/**/*.hpp` 文件由 `register_itools.cpp` 的 `#include` 收集，无需 `.cpp` 文件或 codegen。如果工具不可用，请确保工具头文件已正确放置在 tools 目录下，并且已在对应的 X-macro 注册文件中注册。

## 构建问题

### Windows 构建卡死

使用 `py -3` 而非 `python`。Microsoft Store 的 python 路由桩会导致静默卡死。

### 构建报 MSB4019 / VCTargetsPath 错误

`main.py` 会自动检测 Visual Studio 安装路径并设置 `VCTargetsPath`。如果失败，尝试：

```bash
uv run python main.py build --clean
```

### 插件重新加载失败

Godot 编辑器加载插件时会锁定 DLL。重建前需关闭编辑器或在项目设置中禁用插件。

## 编辑器问题

### @export 变量未注册

挂载脚本时需按顺序调用：

1. `EditorInterface::get_resource_filesystem()->update_file(path)`
2. `ResourceLoader::load(path)`
3. `GDScript::set_source_code(src)`
4. `GDScript::reload()`

缺少任一步骤都可能导致 `@export` 变量不显示。

### create_node 撤销后节点仍然存在

`create_node` 的撤销操作必须在 `EditorUndoRedoManager` 中调用 `add_do_reference(node)`，防止 Godot 在撤销前对新建节点进行垃圾回收。

## 其他

### 为什么使用 C++ 而不是 GDScript？

GDExtension 直接操作 Godot 引擎底层 API，提供更好的性能和更完整的 API 访问能力。

### 如何修改监听端口？

设置环境变量：

```bash
# Windows
set GODOT_MCP_HTTP_PORT=9700

# Linux / macOS
export GODOT_MCP_HTTP_PORT=9700
```

### 提交 Issue 或贡献代码

GitHub 仓库：[https://github.com/jesspig/GodotMCP-GDExtension](https://github.com/jesspig/GodotMCP-GDExtension)
