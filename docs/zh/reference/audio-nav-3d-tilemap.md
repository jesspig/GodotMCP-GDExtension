# 音频、导航、3D 场景与 TileMap 工具

12 个用于音频、导航、3D 场景和 TileMap 的工具。

## 音频（3 个）

### `create_audio_player`

**分类**: `editor_tools/audio`  
**描述**: 创建 AudioStreamPlayer2D 或 AudioStreamPlayer3D。  
**破坏性**: 否 | **撤销**: 是 | **需要场景**: 是

#### 参数

| 名称 | 类型 | 必需 | 默认值 | 说明 |
|------|------|----------|---------|-------------|
| `parent_path` | `string` | 是 | - | 父节点路径 |
| `node_name` | `string` | 否 | auto | 节点名称 |
| `is_3d` | `bool` | 否 | false | 是否使用 3D 音频播放器 |

### `set_audio_stream`

**分类**: `editor_tools/audio`  
**描述**: 在 AudioStreamPlayer 上设置音频流。  
**破坏性**: 否 | **撤销**: 是 | **需要节点**: 是

#### 参数

| 名称 | 类型 | 必需 | 说明 |
|------|------|----------|-------------|
| `node_path` | `string` | 是 | AudioStreamPlayer 路径 |
| `stream_path` | `string` | 是 | 音频流资源路径 |

### `list_audio_buses`

**分类**: `editor_tools/audio`  
**描述**: 列出音频总线及其效果。  
**破坏性**: 否 | **撤销**: 否

---

## 导航（3 个）

### `create_navigation_region`

**分类**: `editor_tools/navigation`  
**描述**: 创建 NavigationRegion2D 或 NavigationRegion3D。  
**破坏性**: 否 | **撤销**: 是 | **需要场景**: 是

### `create_navigation_agent`

**分类**: `editor_tools/navigation`  
**描述**: 创建 NavigationAgent2D 或 NavigationAgent3D。  
**破坏性**: 否 | **撤销**: 是 | **需要场景**: 是

### `bake_navigation_mesh`

**分类**: `editor_tools/navigation`  
**描述**: 烘焙导航网格。  
**破坏性**: 否 | **撤销**: 否 | **需要场景**: 是

#### 参数

| 名称 | 类型 | 必需 | 说明 |
|------|------|----------|-------------|
| `region_path` | `string` | 是 | NavigationRegion 节点路径 |

---

## 3D 场景（3 个）

### `create_mesh_instance_3d`

**分类**: `editor_tools/3d_scene`  
**描述**: 创建 MeshInstance3D。  
**破坏性**: 否 | **撤销**: 是 | **需要场景**: 是

#### 参数

| 名称 | 类型 | 必需 | 默认值 | 说明 |
|------|------|----------|---------|-------------|
| `parent_path` | `string` | 是 | - | 父节点路径 |
| `mesh_type` | `string` | 否 | "BoxMesh" | 网格类型 |
| `node_name` | `string` | 否 | auto | 节点名称 |

### `create_light_3d`

**分类**: `editor_tools/3d_scene`  
**描述**: 创建 DirectionalLight3D。  
**破坏性**: 否 | **撤销**: 是 | **需要场景**: 是

### `set_world_environment`

**分类**: `editor_tools/3d_scene`  
**描述**: 设置 WorldEnvironment。  
**破坏性**: 否 | **撤销**: 是 | **需要场景**: 是

---

## TileMap（3 个）

### `get_tilemap_info`

**分类**: `editor_tools/tilemap`  
**描述**: 获取 TileMap 信息。  
**破坏性**: 否 | **撤销**: 否 | **需要节点**: 是

### `set_tilemap_cells`

**分类**: `editor_tools/tilemap`  
**描述**: 设置 TileMap 单元格。  
**破坏性**: 是 | **撤销**: 是 | **需要节点**: 是

### `erase_tilemap_cells`

**分类**: `editor_tools/tilemap`  
**描述**: 擦除 TileMap 单元格。  
**破坏性**: 是 | **撤销**: 是 | **需要节点**: 是