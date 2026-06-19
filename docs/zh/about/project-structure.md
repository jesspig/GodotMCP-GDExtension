# 项目结构

## 顶层布局

```
GodotMCP/
extensions/              C++ GDExtension 源码
  CMakeLists.txt         GDExtension 构建配置（godot-cpp 10.0.0-rc1）
  src/                   所有 C++ 源代码
docs/                    文档源文件（Rspress）
  en/                    英文文档
  zh/                    中文文档
  public/                静态资源
doc_build/               构建后的文档输出
example/                 Godot 测试项目
  addons/godot_mcp/      插件文件（由构建自动生成）
tests/                   YAML 测试框架
  yaml_tests/            26 个 YAML 测试文件
  test_orchestrator.py   Python 测试编排器
  godot_manager.py       Godot 进程生命周期管理
scripts/                 Python 构建/打包脚本
cmake/                   CMake 模块（8 个文件）
main.py                  构建/测试/打包入口点
rspress.config.ts        文档站点配置
CMakeLists.txt           根 CMake 构建（版本：0.2.2-dev1）
```

## 源代码布局（extensions/src/）

```
extensions/src/
register_types.cpp          GDExtension 入口（符号：gdext_mcp_init）
editor_plugin.cpp/.hpp      EditorPlugin 生命周期 + _process() 主循环
client_config_registry.hpp  MCP 客户端配置模板（11 个提供商）

sdk/
  mcp_tool_definition       SDK 基类（GDScript/C# 可继承）
  mcp_tool_registry         Engine 单例注册表

server/
  ipc/                      HTTP 服务器层
  mcp/                      MCP 协议处理器（无会话）
  registry/                 HandlerRegistry（工具分发 + 搜索）

built_in/
  tool_base                 ITool 基类 + 类型校验
  tool_adapter              SDK 到 ITool 适配器
  cmd_utils                 共享工具函数
  cmd_utils_json            JSON 到 Variant 转换
  cmd_utils/                7 个模板头文件
  register_itools.cpp       X-macro 注册主文件
  tools/                    153 个工具实现
    meta/                   7 个元工具
    signal/                 4 个信号工具
    group/                  4 个分组工具
    node_tools/             6 个资源 + 2 个兜底
    editor_tools/           19 个子分类
    runtime_tools/          7 个桥接 + 6 个生命周期

runtime/
  bridge                    编辑器端 TCP 客户端
  game_bridge               游戏进程 TCP 服务端

ui/
  mcp_dock                  编辑器右侧面板停靠栏
  mcp_logger                C++ 结构化日志器
  mcp_console               编辑器输出控制台

testing/
  test_engine               YAML 流水线测试引擎
  pipeline_parser           流水线 YAML 解析器
  pipeline_runner           流水线执行器
  pipeline_context          流水线上下文
  pipeline_types            类型定义
  yaml_parser               YAML 解析工具
  test_assertions           断言辅助函数
  godot_file_verifier       磁盘文件校验
```