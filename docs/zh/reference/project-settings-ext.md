# 项目设置扩展

> 10 个聚合设置工具，将分散在 `ProjectSettings` 中的相关属性打包为 get/set 对。

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

## ProjectSettings 键映射

### 显示设置

| 字段 | ProjectSettings 键 |
|-------------|-------------------|
| viewport_width | `display/window/size/viewport_width` |
| viewport_height | `display/window/size/viewport_height` |
| mode | `display/window/size/mode` |
| stretch_mode | `display/window/stretch/mode` |
| stretch_aspect | `display/window/stretch/aspect` |
| vsync_mode | `display/window/vsync/vsync_mode` |

### 物理设置

| 字段 | ProjectSettings 键 |
|-------------|-------------------|
| gravity_2d | `physics/2d/default_gravity` |
| gravity_3d | `physics/3d/default_gravity` |
| linear_damp_2d | `physics/2d/default_linear_damp` |
| physics_ticks_per_second | `physics/common/physics_ticks_per_second` |

### 渲染设置

| 字段 | ProjectSettings 键 |
|-------------|-------------------|
| rendering_method | `rendering/renderer/rendering_method` |
| msaa_2d | `rendering/anti_aliasing/quality/msaa_2d` |
| msaa_3d | `rendering/anti_aliasing/quality/msaa_3d` |
| use_occlusion_culling | `rendering/occlusion_culling/use_occlusion_culling` |

### 层名称

`category` 参数值：`2d_physics`、`2d_navigation`、`2d_render`、`3d_physics`、`3d_navigation`、`3d_render`、`avoidance`

对应键：`layer_names/{category}/layer_{n}`
