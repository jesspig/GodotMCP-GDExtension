# Scene / Node / Property Command Patterns

## JSON↔Variant Conversion

All tools use `j2v` / `v2j` to convert between Godot `Variant` and serde JSON `Value`.

### `j2v` Supported Conversions

| JSON Type | Godot Type | Notes |
|-----------|-----------|-------|
| `null` | `Variant::nil()` | |
| `true`/`false` | `bool` | |
| `number` | `f64` → `Variant` | |
| `string` | `String` | Resource paths loaded via `try_load` |
| `array` | `Variant::array()` | recursive |
| `object` | `Dictionary` | |
| Special | | |
| `{"__type":"Vector2","x":1,"y":2}` | `Vector2` | same for 3/4 |
| `{"__type":"Color","r":1.0,"g":0.0,"b":0.0,"a":1.0}` | `Color` | |
| `{"__type":"Rect2","x":0,"y":0,"w":10,"h":20}` | `Rect2` | |
| `{"__type":"Quaternion","x":0,"y":0,"z":0,"w":1}` | `Quaternion` | |
| `{"__type":"Resource","path":"res://..."}` | loaded via try_load | placeholder on failure |

### `v2j` Supported Conversions

| Godot Type | JSON Type | Notes |
|-----------|-----------|-------|
| `i64`/`f64` | `number` | |
| `bool` | `bool` | |
| `String`/`StringName`/`NodePath` | `string` | |
| `Vector2/3/4` | `{"__type":"VectorX",...}` | |
| `Color` | `{"__type":"Color",...}` | |
| `Rect2` | `{"__type":"Rect2",...}` | |
| `Quaternion` | `{"__type":"Quaternion",...}` | |
| `Resource` | `{"__type":"Resource","path":"..."}` | path only |
| `Array` | `array` | |
| `Dictionary` | `object` | |
| `Variant::nil()` | `JSON::null()` | |

## Node Path Resolution

`resolve_node(&root, path)` supports multiple path formats:

| Input | Behavior |
|-------|----------|
| `""` or `"."` | returns `root` |
| `"/"` | returns `root` |
| `"/root"` | returns `root` |
| `"RootName"` | matches root node `name` |
| `"RootName/Child/Grandchild"` | auto-strips prefix, walks from root |
| Any `NodePath` | normal Godot NodePath resolution |

## Scene File Operations

### Read / Write Scene Files

- **Read**: `ResourceLoader::load(...)` → cast to scene root node
- **Write**: Use `EditorInterface::singleton()` methods — direct `.tscn` file writes won't be detected by editor

### EditorInterface Scene Methods

| Method | Tool |
|--------|------|
| `edit_node(node)` | Select node in current scene |
| `get_current_scene()` | Get current scene root |
| `save_scene()` | Save current scene |
| `save_scene_as(path)` | Save as |
| `open_scene_from_path(path)` | Open in editor tab |
| `reload_scene_from_path(path)` | Reload from disk |
| `mark_scene_as_unsaved()` | Mark as unsaved |
| `get_edited_scene_root()` | Get edited scene root node |

### Scene File Lifecycle

```
open_scene(path)
  ↓
EditorInterface opens file → tab visible
  ↓
get_open_scene_roots() / get_open_scenes()
  ↓
Modify commands → auto-mark dirty
  ↓
save_scene() / save_scene_as(path)
  ↓
close_scene() → close tab
```

## Node Instantiation

`instantiate_scene(path, parent)`:
1. Load scene from `res://` path
2. `PackedScene::instantiate()` creates node
3. `parent.add_child(node)` adds it
4. `EditorInterface::edit_node(node)` selects it

`branch_to_scene(node_path, scene_path)`:
1. Duplicate source node
2. Create new `PackedScene`
3. Save to file
4. `EditorInterface::get_resource_filesystem().update_file()` notify

## Resource Loading

Prefer `try_load::<T>(path)`:

```rust
// Don't:
let res = ResourceLoader::singleton().load("res://icon.svg");
let tex = res.unwrap().cast::<Texture2D>();

// Do:
let tex = try_load::<Texture2D>("res://icon.svg")?;
```

## Filesystem Notifications

After writing files:

```rust
EditorInterface::singleton()
    .get_resource_filesystem()
    .update_file(path);
```

This ensures the editor detects filesystem changes.

## Directory Creation

```rust
let dir = DirAccess::open("res://")?;
dir.make_dir_recursive("new/subdir/path")?;
```

## Batch Property Setting

`batch_set_property` accepts node paths, property name, and value; loops over each node — all in a single undo action.
