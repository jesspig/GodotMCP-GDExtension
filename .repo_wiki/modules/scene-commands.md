# Scene / Node / Property 命令模式

## JSON↔Variant 转换

所有工具使用 `j2v` / `v2j` 在 Godot `Variant` 和 serde JSON `Value` 之间转换。

### `j2v` 支持的转换

| JSON 输入 | Godot 类型 | 检测方式 |
|-----------|-----------|----------|
| `null` | `Variant::nil()` | 直接匹配 |
| `true`/`false` | `bool` | `try_to::<bool>()` |
| `number`（整数） | `i64` | `as_i64()` |
| `number`（浮点） | `f64` | `as_f64()` |
| `string` | `GString` | 默认 |
| `"res://..."` / `"user://..."` | `Gd<Resource>`（通过 `try_load`） | 字符串前缀检查 |
| `{"x":1,"y":2}` | `Vector2` | 精确 2 字段匹配 |
| `{"x":1,"y":2,"z":3}` | `Vector3` | 精确 3 字段匹配 |
| `{"x":1,"y":2,"z":3,"w":4}` | `Quaternion` | 精确 4 字段匹配 |
| `{"r":1,"g":0,"b":0,"a":1}` | `Color` (RGBA) | 精确 4 字段匹配 |
| `{"r":1,"g":0,"b":0}` | `Color` (RGB) | 精确 3 字段匹配 |
| `{"position":{...},"size":{...}}` | `Rect2` | 精确 2 字段匹配 |
| `{"resource_path":"res://..."}` | `Gd<Resource>`（通过 `try_load`） | 特定 key 匹配 |
| `[1,2]` | `Vector2` | float 数组，len=2 |
| `[1,2,3]` | `Vector3` | float 数组，len=3 |
| `[1,2,3,4]` | `Color` | float 数组，len=4 |

**注意**：旧版文档提到的 `{"__type":"Vector2",...}` 格式不再使用——改用纯字段名检测。

### `v2j` 支持的转换

| Godot 类型 | JSON 输出 |
|-----------|-----------|
| `nil` | `null` |
| `bool` | `true`/`false` |
| `i64` | `number` |
| `f64` | `number` |
| `GString` | `string` |
| `Vector2` | `{"x":..., "y":...}` |
| `Vector3` | `{"x":..., "y":..., "z":...}` |
| `Vector4` | `{"x":..., "y":..., "z":..., "w":...}` |
| `Color` | `{"r":..., "g":..., "b":..., "a":...}` |
| `Rect2` | `{"position":{"x":...,"y":...},"size":{"x":...,"y":...}}` |
| `Quaternion` | `{"x":..., "y":..., "z":..., "w":...}` |
| `Gd<Resource>` | `{"resource_path":"res://..."}`（有路径时）或 string |
| 其他 | `v.to_string()` 兜底 |

## 节点路径解析

`resolve_node(&root, path)` 支持多种路径格式：

| 输入 | 行为 |
|------|------|
| `""` 或 `"."` | 返回 `root` |
| `"/"` | 返回 `root` |
| `"/root"` | 返回 `root` |
| `"RootName"` | 匹配根节点的 `name` 属性 |
| `"RootName/Child/Grandchild"` | 前缀自动剥离后从根节点开始查找 |
| 任意 `NodePath` | 正常的 Godot NodePath 解析 |

## 场景文件操作

### 读 / 写场景文件

- **读取**: `ResourceLoader::load(...)` → 强制转换场景的根节点
- **写入**: 使用 `EditorInterface::singleton()` 方法 —— 直接写 `.tscn` 文件编辑器不会检测到

### EditorInterface 场景方法

| 方法 | 工具 |
|------|------|
| `edit_node(node)` | 选择当前场景中的节点 |
| `get_current_scene()` | 获取当前打开场景的根节点 |
| `save_scene()` | 保存当前场景 |
| `save_scene_as(path)` | 另存为 |
| `open_scene_from_path(path)` | 在编辑器标签中打开 |
| `reload_scene_from_path(path)` | 重新加载 |
| `mark_scene_as_unsaved()` | 标记为未保存 |
| `get_edited_scene_root()` | 获取编辑中的场景根节点 |

### 场景文件生命周期

```
open_scene(path)
  ↓
EditorInterface 打开文件 → 标签页可见
  ↓
get_open_scene_roots() / get_open_scenes()
  ↓
修改命令 → 自动标记脏
  ↓
save_scene() / save_scene_as(path)
  ↓
close_scene() → 关闭标签页
```

## 节点实例化

`instantiate_scene(path, parent)`:
1. 从 `res://` 路径加载场景
2. `PackedScene::instantiate()` 创建节点
3. `parent.add_child(node)` 添加
4. `EditorInterface::edit_node(node)` 选择新节点

`branch_to_scene(node_path, scene_path)`:
1. 复制源节点
2. 创建新 `PackedScene`
3. 保存到文件
4. `EditorInterface::get_resource_filesystem().update_file()` 通知文件系统

## 资源加载

优先使用 `try_load::<T>(path)`：

```rust
// 不要这样：
let res = ResourceLoader::singleton().load("res://icon.svg");
let tex = res.unwrap().cast::<Texture2D>();

// 应该这样：
let tex = try_load::<Texture2D>("res://icon.svg")?;
```

## 文件系统通知

写入文件后调用：

```rust
EditorInterface::singleton()
    .get_resource_filesystem()
    .update_file(path);
```

这样编辑器才会检测到文件系统的变化。

## 目录创建

```rust
let dir = DirAccess::open("res://")?;
dir.make_dir_recursive("new/subdir/path")?;
```

## 批量属性设置

`batch_set_property` 接受节点路径列表、属性和值，对每个节点循环设置，全部在一个 undo action 中。

## Undoable 属性设置

`undoable_set(node, property, new_value, action_name)`:
1. 读取旧值
2. 立即应用新值（修改节点）
3. 通过 `EditorUndoRedoManager` 记录 do/undo 操作

## 共享工具函数

| 函数 | 说明 |
|------|------|
| `ensure_parent_dir(path)` | 创建 `res://` 路径的父目录（**主线程调用**） |
| `relative_path(node, root)` | 编辑器路径 → 场景相对路径 |
| `get_root()` | 获取当前编辑场景根节点 |
| `mark_dirty()` | 标记场景未保存 |
| `fix_owners_recursive(node, owner)` | 递归修正节点 owner |
| `node_replace_owner(base, old, new, ur, mode)` | UndoRedo 安全的 owner 替换 |