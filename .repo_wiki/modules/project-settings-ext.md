# Project Settings Extensions

> `crates/gdext/src/commands/project_settings_ext.rs` — 10 个聚合设置工具，将分散在 `ProjectSettings` 中的相关属性打包为 get/set 对。

## 工具列表

| 工具 | 分类 | 说明 |
|------|------|------|
| `get_display_settings` | 显示 | 读取所有显示/窗口相关设置 |
| `set_display_settings` | 显示 | 批量设置窗口模式、分辨率、拉伸模式等 |
| `get_project_info` | 项目信息 | 读取项目基本信息 |
| `set_project_info` | 项目信息 | 设置名称、描述、版本、图标、主场景等 |
| `get_physics_settings` | 物理 | 读取物理引擎参数 |
| `set_physics_settings` | 物理 | 设置 2D/3D 重力、阻尼、物理帧率 |
| `get_rendering_settings` | 渲染 | 读取渲染参数 |
| `set_rendering_settings` | 渲染 | 设置渲染方法、MSAA、抗锯齿等 |
| `get_layer_names` | 层名称 | 读取各分类的层名称映射 |
| `set_layer_names` | 层名称 | 设置层号到名称的映射 |

## 实现模式

所有工具遵循同一模式：

```rust
// 通过 ProjectSettings::singleton().get_setting("key") 读取
// 通过 ProjectSettings::singleton().set_setting("key", value) 写入
// 设置后调用 ProjectSettings::singleton().save() 持久化
```

```mermaid
flowchart LR
    subgraph Read["读取"]
        A1[get_display_settings]
        A2[get_project_info]
        A3[get_physics_settings]
        A4[get_rendering_settings]
        A5[get_layer_names]
    end
    subgraph Write["写入"]
        B1[set_display_settings]
        B2[set_project_info]
        B3[set_physics_settings]
        B4[set_rendering_settings]
        B5[set_layer_names]
    end
    subgraph PS["ProjectSettings"]
        C["get_setting() / set_setting()"]
    end
    Read --> PS
    Write --> PS
    PS -->|save()| D["project.godot"]
```

## 各分类的 ProjectSettings 键映射

### 显示设置

| get/set 字段 | ProjectSettings 键 |
|-------------|-------------------|
| viewport_width | `display/window/size/viewport_width` |
| viewport_height | `display/window/size/viewport_height` |
| mode | `display/window/size/mode` |
| resizable | `display/window/size/resizable` |
| borderless | `display/window/size/borderless` |
| transparent | `display/window/size/transparent` |
| always_on_top | `display/window/size/always_on_top` |
| stretch_mode | `display/window/stretch/mode` |
| stretch_aspect | `display/window/stretch/aspect` |
| stretch_scale | `display/window/stretch/scale` |
| vsync_mode | `display/window/vsync/vsync_mode` |

### 项目信息

| get/set 字段 | ProjectSettings 键 |
|-------------|-------------------|
| name | `application/config/name` |
| description | `application/config/description` |
| version | `application/config/version` |
| icon | `application/config/icon` |
| main_scene | `application/run/main_scene` |
| use_custom_user_dir | `application/config/use_custom_user_dir` |
| custom_user_dir_name | `application/config/custom_user_dir_name` |

### 物理设置

| get/set 字段 | ProjectSettings 键 |
|-------------|-------------------|
| gravity_2d | `physics/2d/default_gravity` |
| gravity_3d | `physics/3d/default_gravity` |
| linear_damp_2d | `physics/2d/default_linear_damp` |
| angular_damp_2d | `physics/2d/default_angular_damp` |
| linear_damp_3d | `physics/3d/default_linear_damp` |
| angular_damp_3d | `physics/3d/default_angular_damp` |
| physics_ticks_per_second | `physics/common/physics_ticks_per_second` |

### 渲染设置

| get/set 字段 | ProjectSettings 键 |
|-------------|-------------------|
| rendering_method | `rendering/renderer/rendering_method` |
| default_clear_color | `rendering/environment/defaults/default_clear_color` |
| msaa_2d | `rendering/anti_aliasing/quality/msaa_2d` |
| msaa_3d | `rendering/anti_aliasing/quality/msaa_3d` |
| screen_space_aa | `rendering/anti_aliasing/quality/screen_space_aa` |
| snap_2d_transforms_to_pixel | `rendering/2d/snap/snap_2d_transforms_to_pixel` |
| snap_2d_vertices_to_pixel | `rendering/2d/snap/snap_2d_vertices_to_pixel` |
| use_occlusion_culling | `rendering/occlusion_culling/use_occlusion_culling` |

### 层名称

`get_layer_names` 的 `category` 参数值：`2d_physics`、`2d_navigation`、`2d_render`、`3d_physics`、`3d_navigation`、`3d_render`、`avoidance`

对应的 ProjectSettings 键：`layer_names/{category}/layer_{n}`

## 注意

- 所有设置操作最终会调用 `ProjectSettings::save()`，失败时 `warning` 字段会有提示
- `set_project_info` 中的 icon 路径使用 `try_load::<CompressedTexture2D>` 加载验证
