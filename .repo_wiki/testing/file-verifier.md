# 磁盘文件验证（旧——将被 C++ 引擎替代）

> `file_verifier.py`——无需 Godot 编辑器即可直接解析 Godot 项目文件（`.tscn`、`project.godot`）的纯 Python 工具。将被 C++ `godot_file_verifier.hpp` 替代。

## 用途

测试阶段在通过 MCP 创建/修改场景后，调用 `file_verifier` 直接读取 `.tscn` 文件，验证磁盘上的节点属性是否与 MCP 返回值一致。这提供了**双通道验证**：运行时 API 返回值 + 磁盘持久化状态。

## 核心函数

### `resolve_path(path: str, project_path: str) -> str`

将 `res://` 路径转换为文件系统绝对路径。

### `file_exists(path: str, project_path: str) -> bool`

检查 `res://` 或相对路径对应的文件是否存在。

### `file_content(path: str, project_path: str) -> str`

读取文件内容为字符串。

### `parse_tscn(filepath: str) -> dict`

完整解析 `.tscn`（文本场景）文件，返回结构化字典：

```python
{
    "gd_scene": {"format": 3, "uid": "uid://..."},
    "ext_resources": [{"path": "res://icon.svg", "type": "Texture2D", "id": 1}],
    "sub_resources": [{"type": "RectangleShape2D", "id": "SubResource_1", "props": {"size": Vector2(100, 100)}}],
    "nodes": [
        {
            "type": "Node2D",
            "name": "MyNode",
            "parent": ".",
            "props": {"position": Vector2(100, 200), "rotation": 0.0},
            "named_props": {"position": {"x": 100.0, "y": 200.0}}  # Vector2 → {x,y}
        }
    ],
    "connections": [{"signal": "pressed", "from": "...", "to": "...", "method": "..."}]
}
```

### `get_node_property(tscn: dict, node_name: str, prop_key: str) -> Any`

在解析的 tscn 字典中按名称和属性键查找节点属性值。

### `verify_tscn_has_node(tscn: dict, name: str, node_type: str = None) -> bool`

检查 `.tscn` 文件中是否存在指定名称（和可选类型）的节点。

### `verify_project_godot(content: str, section: str, key: str, expected: str) -> bool`

检查 `project.godot` 文件中指定章节/键的值是否匹配预期。

### `read_project_godot_value(content: str, section: str, key: str) -> Optional[str]`

从原始字符串内容中读取 `project.godot` 的某个配置值。

### `verify_file_contains(content: str, substring: str) -> bool`

检查文件内容中是否包含子字符串。

### `_parse_bracket_attrs(line: str) -> dict`

内部函数，从 `[node name="Foo" type="Bar"]` 格式的括号部分解析键值属性。支持带引号和不带引号的值。
