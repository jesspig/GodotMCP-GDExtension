# Audio, Navigation, 3D Scene & TileMap Tools

12 tools for audio, navigation, 3D scenes, and tilemaps.

## Audio (3)

### `create_audio_player`

**Category**: `editor_tools/audio`  
**Description**: Create an AudioStreamPlayer2D or AudioStreamPlayer3D.  
**Destructive**: No | **Undo**: Yes | **Needs Scene**: Yes

#### Parameters

| Name | Type | Required | Default | Description |
|------|------|----------|---------|-------------|
| `parent_path` | `string` | Yes | - | Parent node path |
| `node_name` | `string` | No | auto | Node name |
| `is_3d` | `bool` | No | false | Use 3D audio player |

### `set_audio_stream`

**Category**: `editor_tools/audio`  
**Description**: Set an audio stream on an AudioStreamPlayer.  
**Destructive**: No | **Undo**: Yes | **Needs Node**: Yes

#### Parameters

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `node_path` | `string` | Yes | AudioStreamPlayer path |
| `stream_path` | `string` | Yes | Audio stream resource path |

### `list_audio_buses`

**Category**: `editor_tools/audio`  
**Description**: List audio buses and their effects.  
**Destructive**: No | **Undo**: No

---

## Navigation (3)

### `create_navigation_region`

**Category**: `editor_tools/navigation`  
**Description**: Create a NavigationRegion2D or NavigationRegion3D.  
**Destructive**: No | **Undo**: Yes | **Needs Scene**: Yes

### `create_navigation_agent`

**Category**: `editor_tools/navigation`  
**Description**: Create a NavigationAgent2D or NavigationAgent3D.  
**Destructive**: No | **Undo**: Yes | **Needs Scene**: Yes

### `bake_navigation_mesh`

**Category**: `editor_tools/navigation`  
**Description**: Bake a navigation mesh.  
**Destructive**: No | **Undo**: No | **Needs Scene**: Yes

#### Parameters

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `region_path` | `string` | Yes | NavigationRegion node path |

---

## 3D Scene (3)

### `create_mesh_instance_3d`

**Category**: `editor_tools/3d_scene`  
**Description**: Create a MeshInstance3D.  
**Destructive**: No | **Undo**: Yes | **Needs Scene**: Yes

#### Parameters

| Name | Type | Required | Default | Description |
|------|------|----------|---------|-------------|
| `parent_path` | `string` | Yes | - | Parent node path |
| `mesh_type` | `string` | No | "BoxMesh" | Mesh type |
| `node_name` | `string` | No | auto | Node name |

### `create_light_3d`

**Category**: `editor_tools/3d_scene`  
**Description**: Create a DirectionalLight3D.  
**Destructive**: No | **Undo**: Yes | **Needs Scene**: Yes

### `set_world_environment`

**Category**: `editor_tools/3d_scene`  
**Description**: Set WorldEnvironment.  
**Destructive**: No | **Undo**: Yes | **Needs Scene**: Yes

---

## TileMap (3)

### `get_tilemap_info`

**Category**: `editor_tools/tilemap`  
**Description**: Get TileMap info.  
**Destructive**: No | **Undo**: No | **Needs Node**: Yes

### `set_tilemap_cells`

**Category**: `editor_tools/tilemap`  
**Description**: Set TileMap cells.  
**Destructive**: Yes | **Undo**: Yes | **Needs Node**: Yes

### `erase_tilemap_cells`

**Category**: `editor_tools/tilemap`  
**Description**: Erase TileMap cells.  
**Destructive**: Yes | **Undo**: Yes | **Needs Node**: Yes