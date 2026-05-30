# C# Solution Generation

> Generates `.sln` + `.csproj` files directly within the GDExtension, without launching a second Godot process.

## File Structure

```
res://
├── project_name.sln          # Visual Studio Solution (UTF-8 with BOM)
└── project_name.csproj       # C# Project (UTF-8 without BOM)
```

## Version Derivation

Base version from `Engine::get_version_info` gets `{major}.{minor}.{patch}`. Status handling: SemVer 2.0 requires pre-release tags to use dot separators, e.g., `"beta2"` → `"beta.2"`.

## Project Name Selection (by priority)

1. `dotnet/project/assembly_name` in `ProjectSettings`
2. `application/config/name` value
3. Project folder name (fallback)

## Tool List

| Tool | Description |
|------|-------------|
| `csharp_create_solution` | Generate `.sln` + `.csproj` (can enable NativeAOT) |
| `csharp_build` | Run `dotnet build` to compile C# project |
| `create_csharp_script` | Create a C# script file from template |
| `read_csharp_script` | Read C# script source |
| `edit_csharp_script` | Edit C# script source |
| `list_csharp_scripts` | List C# script files in the project |

## Notes

- `csharp_build` calls `dotnet build` — cannot run when the editor holds the assembly file lock
- `csharp_create_solution` must be called before creating C# scripts
- `.csproj` uses UTF-8 without BOM; `.sln` uses UTF-8 with BOM (required by VS)
