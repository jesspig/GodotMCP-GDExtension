# Project Settings Extensions

> 10 aggregated setting tools that bundle related properties scattered across `ProjectSettings` into get/set pairs.

## Tool List

| Tool | Category | Description |
|------|----------|-------------|
| `get_display_settings` | Display | Read all display/window settings |
| `set_display_settings` | Display | Batch set window mode, resolution, stretch mode, etc. |
| `get_project_info` | Project Info | Read project basic info |
| `set_project_info` | Project Info | Set name, description, version, icon, main scene, etc. |
| `get_physics_settings` | Physics | Read physics engine parameters |
| `set_physics_settings` | Physics | Set 2D/3D gravity, damping, physics ticks per second |
| `get_rendering_settings` | Rendering | Read rendering parameters |
| `set_rendering_settings` | Rendering | Set rendering method, MSAA, anti-aliasing, etc. |
| `get_layer_names` | Layer Names | Read layer name mappings for each category |
| `set_layer_names` | Layer Names | Set layer number to name mappings |

## ProjectSettings Key Mapping

### Display Settings

| Field | ProjectSettings Key |
|-------|---------------------|
| viewport_width | `display/window/size/viewport_width` |
| viewport_height | `display/window/size/viewport_height` |
| mode | `display/window/size/mode` |
| stretch_mode | `display/window/stretch/mode` |
| stretch_aspect | `display/window/stretch/aspect` |
| vsync_mode | `display/window/vsync/vsync_mode` |

### Physics Settings

| Field | ProjectSettings Key |
|-------|---------------------|
| gravity_2d | `physics/2d/default_gravity` |
| gravity_3d | `physics/3d/default_gravity` |
| linear_damp_2d | `physics/2d/default_linear_damp` |
| physics_ticks_per_second | `physics/common/physics_ticks_per_second` |

### Rendering Settings

| Field | ProjectSettings Key |
|-------|---------------------|
| rendering_method | `rendering/renderer/rendering_method` |
| msaa_2d | `rendering/anti_aliasing/quality/msaa_2d` |
| msaa_3d | `rendering/anti_aliasing/quality/msaa_3d` |
| use_occlusion_culling | `rendering/occlusion_culling/use_occlusion_culling` |

### Layer Names

`category` parameter values: `2d_physics`, `2d_navigation`, `2d_render`, `3d_physics`, `3d_navigation`, `3d_render`, `avoidance`

Corresponding keys: `layer_names/{category}/layer_{n}`
