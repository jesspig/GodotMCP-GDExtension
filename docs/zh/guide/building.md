# 从源码构建

## 前置条件

- **Python 3.14+**（查看 `.python-version`）
- **uv**（Python 包管理器）
- **CMake 3.25+**
- **C++17 编译器**（MSVC、GCC、Clang 或 AppleClang）
- **Git**（用于 FetchContent 依赖）

## 快速构建

```bash
uv run python main.py build
```

该命令执行 Debug 构建并将结果复制到 `example/addons/godot_mcp/bin/`。

## 构建选项

### Release 构建

```bash
uv run python main.py build --release
```

### 构建并打包

```bash
uv run python main.py build --zip
```

构建后打包 `addons.zip` 用于分发。

### 并行编译

```bash
uv run python main.py build -j 16
```

### 自定义编译器/生成器

```bash
uv run python main.py build --compiler clang-cl --generator ninja
```

### 编译器缓存

```bash
uv run python main.py build --compiler clang-cl --generator ninja --cache sccache
```

## 构建命令参考

| 命令 | 描述 |
|---------|-------------|
| `uv run python main.py build` | Debug 构建 |
| `uv run python main.py build --release` | Release 构建 |
| `uv run python main.py build --zip` | 构建 + 打包 addons.zip |
| `uv run python main.py build --clean` | 清理构建缓存（保留依赖） |
| `uv run python main.py package` | 将预编译库打包为 addons.zip |
| `uv run python main.py test` | 完整测试流水线 |

## CI 配方

### Windows（clang-cl + ninja + sccache）

```bash
uv run python main.py build --compiler clang-cl --generator ninja --cache sccache
```

### Linux（gcc + ninja + sccache）

```bash
uv run python main.py build --compiler gcc --generator ninja --cache sccache
```

### macOS（appleclang + ninja + sccache）

```bash
uv run python main.py build --compiler appleclang --generator ninja --cache sccache
```

## 重要提示

- **DLL 锁**：Godot 编辑器会锁定 DLL。重新构建前请先关闭编辑器。
- **依赖**：`godot-cpp 10.0.0-rc1` 和 `ryml v0.7.0` 通过 CMake FetchContent 获取。
- **版本号**：唯一来源为根目录 `CMakeLists.txt` 中的 `PROJECT_VERSION`。
- **自动生成配置**：`plugin.cfg` 和 `.gdextension` 每次构建都会重新生成。