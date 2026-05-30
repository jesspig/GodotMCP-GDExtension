---
pageType: home

hero:
  name: GodotMCP
  tagline: 通过 Model Context Protocol 将 Godot 4.6+ 编辑器接入 AI 编码助手
  actions:
    - text: 快速开始
      link: /guide/getting-started
      theme: brand
    - text: 工具目录
      link: /reference/tools-catalog
      theme: alt
    - text: 更新日志
      link: /changelog/
      theme: alt

features:
  - title: 🎮 122 个编辑器工具
    details: 涵盖场景管理、节点操作、脚本编辑、资源控制、项目设置、输入映射等，全面覆盖 Godot 编辑器操作
  - title: ⚡ 原生 C++ 性能
    details: 基于 C++ GDExtension 实现，零 GC 暂停，纯主线程轮询，无锁无竞争，编辑器零感知延迟
  - title: 🔌 Streamable HTTP
    details: 通过 JSON-RPC 2.0 + SSE 提供流式工具调用，支持任意 MCP 客户端（Claude Code、Continue、opencode 等）
  - title: 🛠️ GDScript / C# 双支持
    details: 自动语法验证（通过 Godot LSP），创建、编辑、编译 C# 解决方案，完整的脚本生命周期管理
  - title: ↩️ 完整 Undo/Redo
    details: 所有编辑器操作集成 Godot 原生撤销系统，每一次修改都可追溯回退
  - title: 🔄 跨平台发布
    details: CI/CD 自动构建 Windows / macOS / Linux 三平台 GDExtension 二进制，Tag 即发布
---
