---
pageType: home
hero:
  name: GodotMCP
  text: 将 AI 编码助手接入 Godot 4.6+ 编辑器
  tagline: 153 个编辑器工具 | C++ GDExtension | MCP Streamable HTTP
  actions:
    - theme: brand
      text: 快速开始
      link: /guide/getting-started
    - theme: alt
      text: 工具目录
      link: /reference/meta-tools
    - theme: alt
      text: 更新日志
      link: /changelog/
  features:
    - title: 153 个内置工具
      details: 涵盖场景树、脚本、文件系统、动画、着色器、运行时桥接等
    - title: C++ GDExtension
      details: 原生性能，运行在编辑器进程内，无外部依赖
    - title: Streamable HTTP
      details: MCP 2026-07-28 协议，端口 9600，纯 POST/OPTIONS，无会话
    - title: 元工具与搜索引擎
      details: 内置 find_tool 搜索引擎，支持渐进式分类发现
    - title: 运行时桥接
      details: 通过 TCP 桥接（端口 9601）查询和控制运行中的游戏实例
    - title: ClassDB 文档查询
      details: 直接查询 Godot 运行时的类数据库，无需维护