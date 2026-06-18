# C# 解决方案生成

> 直接在 GDExtension 中生成 `.sln` + `.csproj` 文件，无需启动第二个 Godot 进程。

## 文件结构

```
res://
├── project_name.sln          # Visual Studio Solution（UTF-8 with BOM）
└── project_name.csproj       # C# Project（UTF-8 without BOM）
```

## 版本推导

基础版本从 `Engine::get_version_info` 获取 `{major}.{minor}.{patch}`，状态处理：SemVer 2.0 要求 pre-release 标签使用点分隔，如 `"beta2"` → `"beta.2"`。

## 项目名选择（按优先级）

1. `ProjectSettings` 中 `dotnet/project/assembly_name` 值
2. `application/config/name` 值
3. 项目文件夹名（兜底）

## 工具列表

| 工具 | 说明 |
|------|------|
| `csharp_create_solution` | 生成 `.sln` + `.csproj`（可启用 NativeAOT） |
| `csharp_build` | 调用 `dotnet build` 编译 C# 项目 |
| `read_csharp_script` | 读取 C# 脚本源码 |
| `write_csharp_script` | 写入/创建 C# 脚本文件 |
| `patch_csharp_script` | 局部修改 C# 脚本源码 |
| `validate_csharp_script` | 验证 C# 脚本语法 |
| `list_csharp_scripts` | 列出项目中的 C# 脚本文件 |

## 注意事项

- `csharp_build` 调用 `dotnet build`——编辑器持有程序集文件锁时无法运行
- `csharp_create_solution` 必须在创建 C# 脚本前执行
- `.csproj` 使用 UTF-8 无 BOM；`.sln` 使用 UTF-8 有 BOM（VS 要求）
