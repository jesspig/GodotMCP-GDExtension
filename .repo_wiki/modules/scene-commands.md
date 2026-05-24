# Scene / Node / Property 命令模式

## JSON↔Variant 转换

所有工具使用 `j2v` / `v2j` 在 Godot `Variant` 和 serde JSON `Value` 之间转换。

### `j2v` 支持的转换

| JSON 类型 | Godot 类型 | 注意 |
|-----------|-----------|------|
| `null` | `Variant::nil()` | |
| `true`/`false` | `bool` | |
| `number` | `f64` → `Variant` | |
| `string` | `String` | Resource 路径需通过 `try_load` 加载 |
| `array` | `Variant::array()` | 递归转换每个元素 |
| `object` | `Dictionary` | |
| 特殊格式 | | |
| `{"__type":"Vector2","x":1,"y":2}` | `Vector2` | 三个坐标格式同理 |
| `{"__type":"Color","r":1.0,"g":0.0,"b":0.0,"a":1.0}` | `Color` | |
| `{"__type":"Rect2","x":0,"y":0,"w":10,"h":20}` | `Rect2` | |
| `{"__type":"Quaternion","x":0,"y":0,"z":0,"w":1}` | `Quaternion` | |
| `{"__type":"Resource","path":"res://..."}` | 通过 `try_load` 加载的资源 | 失败时使用占位符 |

### `v2j` 支持的转换

| Godot 类型 | JSON 类型 | 注意 |
|-----------|-----------|------|
| `i64`/`f64` | `number` | |
| `bool` | `bool` | |
| `String`/`StringName`/`NodePath` | `string` | |
| `Vector2/3/4` | `{"__type":"VectorX",...}` | |
| `Color` | `{"__type":"Color",...}` | |
| `Rect2` | `{"__type":"Rect2",...}` | |
| `Quaternion` | `{"__type":"Quaternion",...}` | |
| `Resource` | `{"__type":"Resource","path":"..."}` | 仅导出路径 |
| `Array` | `array` | |
| `Dictionary` | `object` | |
| `Variant::nil()` | `JSON::null()` | |

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
