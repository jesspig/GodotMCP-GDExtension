# 常见问题

## 连接问题

### GodotMCP 启动后无法连接

1. 确认插件已在项目设置中启用
2. 确认端口未被占用：`curl http://localhost:9600`
3. 检查环境变量 `GODOT_MCP_HTTP_PORT` 是否设置了非默认端口
4. 关闭编辑器后重新打开

### 连接成功后部分工具不可用

工具 schemas 从 `res://addons/godot_mcp/tool_schemas.json` 加载。如果文件不存在或格式错误，工具仍可调用但客户端无参数提示。重新构建插件可修复：

```bash
py -3 build.py --no-zip
```

## 构建问题

### Windows 构建卡死

使用 `py -3` 而非 `python`。Microsoft Store 的 python 路由桩会导致静默卡死。

### 构建报 MSB4019 / VCTargetsPath 错误

build.py 会自动检测 Visual Studio 安装路径并设置 `VCTargetsPath`。如果失败，尝试：

```bash
py -3 build.py --clean
```

### 插件重新加载失败

Godot 编辑器加载插件时会锁定 DLL。重建前需关闭编辑器或在项目设置中禁用插件。

## 编辑器问题

### @export 变量未注册

挂载脚本时需按顺序调用：

1. `EditorInterface::get_resource_filesystem()->update_file(path)`
2. `ResourceLoader::load(path)`
3. `gdscript->set_source_code(src)`
4. `gdscript->reload()`

缺少任一步骤都可能导致 `@export` 变量不显示。

### create_node 撤销后节点仍然存在

`create_node` 的撤销操作必须在 `EditorUndoRedoManager` 中调用 `add_do_reference(node)`，防止 Godot 在撤销前对新建节点进行垃圾回收。

## 其他

### 为什么使用 C++ 而不是 GDScript？

GDExtension 直接操作 Godot 引擎底层 API，提供更好的性能和更完整的 API 访问能力。此外，C++ 编译产物的加载机制更适合实现 HTTP 服务器和 LSP 客户端这类较底层的功能。

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
