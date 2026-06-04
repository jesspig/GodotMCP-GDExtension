#!/usr/bin/env python3
"""
codegen.py — ITool 自动注册代码生成器

扫描 source-dir 下所有 .hpp 文件，查找带有 `// @tool register` 注释的 ITool 子类，
提取类名，生成 `generated_registration.cpp`。

同时支持从 YAML 属性数据库生成 NodePropertyGetTool/NodePropertySetTool 注册代码。

用法:
    uv run python tools/codegen.py \\
        --source-dir extensions/src/built_in/tools \\
        --node-props-db extensions/src/built_in/tools/node_props/db \\
        --output build/generated/generated_registration.cpp
"""

import argparse
import os
import re
import sys

try:
    import yaml
except ImportError:
    yaml = None


# ── @tool register 扫描 ──────────────────────────────────────────────

def find_tool_files(source_dir: str) -> list[dict]:
    """扫描 source_dir 下所有 .hpp 文件，提取工具信息。"""
    if not os.path.isdir(source_dir):
        return []

    tools = []

    for root, _dirs, files in os.walk(source_dir):
        for fname in sorted(files):
            if not fname.endswith(".hpp"):
                continue

            fpath = os.path.join(root, fname)
            try:
                with open(fpath, "r", encoding="utf-8") as f:
                    content = f.read()
            except UnicodeDecodeError:
                print(f"[codegen] WARNING: {fpath} has encoding issues, trying latin-1", file=sys.stderr)
                with open(fpath, "r", encoding="latin-1") as f:
                    content = f.read()

            if "// @tool register" not in content:
                continue

            lines = content.split("\n")
            has_register = False
            class_name = None

            for i, line in enumerate(lines):
                stripped = line.strip()

                if stripped == "// @tool register":
                    has_register = True
                    continue

                m = re.match(
                    r"^class\s+(\w+)\s*[:\n]",
                    stripped,
                )
                if m and class_name is None:
                    class_name = m.group(1)

            if has_register and class_name:
                # source_dir 是 .../src/built_in/tools
                # include 路径要相对 src/：built_in/tools/meta/xxx.hpp
                src_dir = os.path.normpath(os.path.join(source_dir, "..", ".."))
                inc_path = os.path.relpath(fpath, src_dir).replace("\\", "/")

                tools.append({
                    "class_name": class_name,
                    "include_path": inc_path,
                })

    return tools


# ── YAML 属性数据库 ──────────────────────────────────────────────────

def load_node_props_db(db_dir: str) -> dict[str, dict]:
    """加载 YAML 属性数据库，返回 {class_name: {inherits, properties, aliases}}。"""
    if not yaml:
        print("[codegen] WARNING: pyyaml not installed, skipping node props db", file=sys.stderr)
        return {}
    if not os.path.isdir(db_dir):
        return {}

    nodes = {}
    for fname in sorted(os.listdir(db_dir)):
        if not fname.endswith(".yaml") and not fname.endswith(".yml"):
            continue
        fpath = os.path.join(db_dir, fname)
        with open(fpath, "r", encoding="utf-8") as f:
            data = yaml.safe_load(f)
        if not data or "class" not in data:
            continue
        class_name = data["class"]
        nodes[class_name] = {
            "inherits": data.get("inherits", ""),
            "properties": data.get("properties", []),
            "aliases": data.get("aliases", []),
        }

    return nodes


def build_inheritance_chain(node_name: str, nodes: dict[str, dict]) -> list[str]:
    """构建继承链（从根到当前节点），如 ['Node', 'CanvasItem', 'Control', 'Label']。"""
    chain = []
    current = node_name
    while current and current in nodes:
        chain.insert(0, current)
        parent = nodes[current]["inherits"]
        if not parent or parent not in nodes:
            break
        current = parent
    return chain


def get_all_ancestor_props(node_name: str, nodes: dict[str, dict]) -> set[str]:
    """获取所有祖先节点的属性名集合（用于去重）。"""
    ancestor_props = set()
    current = nodes[node_name]["inherits"]
    while current and current in nodes:
        for prop in nodes[current]["properties"]:
            ancestor_props.add(prop["name"])
        current = nodes[current]["inherits"]
    return ancestor_props


def compute_unique_properties(node_name: str, nodes: dict[str, dict]) -> list[dict]:
    """计算当前节点的独有属性（排除祖先已有的属性）。"""
    if node_name not in nodes:
        return []
    ancestor_props = get_all_ancestor_props(node_name, nodes)
    return [p for p in nodes[node_name]["properties"] if p["name"] not in ancestor_props]


def build_inheritance_path(node_name: str, nodes: dict[str, dict]) -> str:
    """构建继承链路径，如 'Node/CanvasItem/Control/Label'。"""
    chain = build_inheritance_chain(node_name, nodes)
    return "/".join(chain)


# ── 代码生成 ─────────────────────────────────────────────────────────

def generate_node_property_registrations(nodes: dict[str, dict]) -> list[str]:
    """生成节点属性工具的注册代码行。"""
    lines = []
    var_idx = 0

    # 按继承链排序：父节点先注册，子节点后注册
    sorted_nodes = []
    for node_name in nodes:
        chain = build_inheritance_chain(node_name, nodes)
        sorted_nodes.append((chain, node_name))
    sorted_nodes.sort(key=lambda x: x[0])

    for chain, node_name in sorted_nodes:
        unique_props = compute_unique_properties(node_name, nodes)
        if not unique_props:
            continue

        cat_path = "node_tools/property/" + build_inheritance_path(node_name, nodes)

        for prop in unique_props:
            prop_name = prop["name"]
            # Get tool
            get_name = f"get_{node_name.lower()}_{prop_name}"
            lines.append(f"    // {node_name}.{prop_name} (get)")
            lines.append(f"    reg.register_tool(std::make_unique<NodePropertyGetTool>(")
            lines.append(f'        "{get_name}", "{cat_path}", "{node_name}", "{prop_name}"));')
            lines.append("")
            # Set tool
            set_name = f"set_{node_name.lower()}_{prop_name}"
            lines.append(f"    // {node_name}.{prop_name} (set)")
            lines.append(f"    reg.register_tool(std::make_unique<NodePropertySetTool>(")
            lines.append(f'        "{set_name}", "{cat_path}", "{node_name}", "{prop_name}"));')
            lines.append("")

        # 别名注册
        aliases = nodes[node_name].get("aliases", [])
        for alias in aliases:
            alias_cat = f"node_tools/property/{build_inheritance_path(node_name, nodes)}/{alias}"
            alias_lower = alias.lower()  # 工具名用小写，如 get_node2d_position
            for prop in unique_props:
                prop_name = prop["name"]
                # Get alias
                alias_get = f"get_{alias_lower}_{prop_name}"
                lines.append(f"    // {alias}.{prop_name} (alias of {node_name}, get)")
                lines.append(f"    reg.register_tool(std::make_unique<NodePropertyGetTool>(")
                lines.append(f'        "{alias_get}", "{alias_cat}", "{node_name}", "{prop_name}"));')
                lines.append("")
                # Set alias
                alias_set = f"set_{alias_lower}_{prop_name}"
                lines.append(f"    // {alias}.{prop_name} (alias of {node_name}, set)")
                lines.append(f"    reg.register_tool(std::make_unique<NodePropertySetTool>(")
                lines.append(f'        "{alias_set}", "{alias_cat}", "{node_name}", "{prop_name}"));')
                lines.append("")

    return lines


def generate_code(tools: list[dict], nodes: dict[str, dict] = None) -> str:
    """生成 generated_registration.cpp 内容。"""
    lines = [
        "// This file is auto-generated by tools/codegen.py",
        "// DO NOT EDIT — all changes will be overwritten.",
        "",
        '#include "server/registry/handler_registry.hpp"',
        '#include "built_in/tool_base.hpp"',
        "",
    ]

    # #include 每个 @tool register 工具头文件
    for t in tools:
        lines.append(f'#include "{t["include_path"]}"')

    # #include 节点属性工具头文件（如果需要）
    if nodes:
        lines.append('#include "built_in/tools/node_props/node_property_tool.hpp"')

    lines.append("")

    lines.append("namespace godot_mcp {")
    lines.append("")
    lines.append("void register_itools(HandlerRegistry &reg) {")

    # 注册 @tool register 工具
    if not tools and not nodes:
        lines.append("    // No ITool-registered tools found.")
    else:
        for i, t in enumerate(tools):
            var_name = f"tool_{i}"
            lines.append(f'    auto {var_name} = std::make_unique<{t["class_name"]}>();')
            lines.append(f'    reg.register_tool(std::move({var_name}));')
            lines.append("")

        # 注册节点属性工具
        if nodes:
            lines.append("    // ── Node Property Tools (auto-generated from YAML) ──")
            lines.append("")
            prop_lines = generate_node_property_registrations(nodes)
            lines.extend(prop_lines)

    lines.append("}")
    lines.append("")
    lines.append("} // namespace godot_mcp")
    lines.append("")

    return "\n".join(lines)


def main():
    parser = argparse.ArgumentParser(description="Generate ITool registration code")
    parser.add_argument(
        "--source-dir",
        required=True,
        help="Directory to scan for tool .hpp files",
    )
    parser.add_argument(
        "--node-props-db",
        default=None,
        help="Directory containing YAML node property database",
    )
    parser.add_argument(
        "--output",
        required=True,
        help="Output file path for generated_registration.cpp",
    )
    args = parser.parse_args()

    tools = find_tool_files(args.source_dir)
    nodes = {}
    if args.node_props_db:
        nodes = load_node_props_db(args.node_props_db)
    code = generate_code(tools, nodes or None)

    os.makedirs(os.path.dirname(args.output), exist_ok=True)
    with open(args.output, "w", encoding="utf-8") as f:
        f.write(code)

    node_count = len(nodes) if nodes else 0
    print(f"[codegen] Found {len(tools)} @tool-register tool(s), {node_count} node type(s), wrote {args.output}")


if __name__ == "__main__":
    main()
