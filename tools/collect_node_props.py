#!/usr/bin/env python3
"""
collect_node_props.py — 从 Godot 运行时提取节点属性数据

通过 Godot 的 --headless 模式运行 GDScript，从 ClassDB 中提取所有节点类型的
独有属性，生成 YAML 文件到 extensions/src/built_in/tools/node_props/db/。

用法:
    uv run python tools/collect_node_props.py --godot /path/to/godot

注意: 需要 Godot 4.6+ 可执行文件。
"""

import argparse
import os
import subprocess
import sys
import yaml


GDSCRIPT_TEMPLATE = """extends SceneTree

func _init():
    var result = []
    var all_classes = ClassDB.get_class_list()
    all_classes.sort()

    for class_name in all_classes:
        if not ClassDB.is_parent_class(class_name, "Node"):
            continue

        # 获取继承链
        var inherits = ClassDB.get_parent_class(class_name)
        if inherits == "Object":
            inherits = ""

        # 获取当前类声明的属性（不含继承的）
        var own_props = []
        var prop_list = ClassDB.class_get_property_list(class_name, true)
        for prop in prop_list:
            if prop.has("usage") and (prop["usage"] & PROPERTY_USAGE_STORAGE != 0):
                # 过滤掉继承的属性
                if not ClassDB.class_has_property(inherits, prop["name"]) if inherits != "" else true:
                    own_props.append({
                        "name": prop["name"],
                        "type": prop["type"]
                    })

        if own_props.size() > 0:
            var node_data = {
                "class": class_name,
                "inherits": inherits,
                "properties": own_props
            }
            result.append(node_data)

    # 输出 YAML
    var file = FileAccess.open("res://_node_props_export.yaml", FileAccess.WRITE)
    for node_data in result:
        file.store_line("---")
        file.store_line("class: " + node_data["class"])
        file.store_line("inherits: " + node_data["inherits"])
        file.store_line("properties:")
        for prop in node_data["properties"]:
            file.store_line("  - name: " + prop["name"])
            file.store_line("    type: " + str(prop["type"]))
    file.close()
    print("Exported " + str(result.size()) + " node types to _node_props_export.yaml")
    quit()
"""


def main():
    parser = argparse.ArgumentParser(
        description="Extract node property data from Godot ClassDB"
    )
    parser.add_argument(
        "--godot",
        required=True,
        help="Path to Godot 4.6+ executable",
    )
    parser.add_argument(
        "--output-dir",
        default=None,
        help="Output directory for YAML files (default: extensions/src/built_in/tools/node_props/db)",
    )
    args = parser.parse_args()

    output_dir = args.output_dir
    if not output_dir:
        # Default to the project's db directory
        script_dir = os.path.dirname(os.path.abspath(__file__))
        output_dir = os.path.join(script_dir, "..", "extensions", "src", "built_in", "tools", "node_props", "db")

    os.makedirs(output_dir, exist_ok=True)

    # Write temporary GDScript
    script_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "_collect_props.gd")
    with open(script_path, "w", encoding="utf-8") as f:
        f.write(GDSCRIPT_TEMPLATE)

    try:
        # Run Godot headless
        print(f"[collect] Running Godot to extract property data...")
        result = subprocess.run(
            [args.godot, "--headless", "--script", script_path],
            capture_output=True,
            text=True,
            timeout=60,
        )

        if result.returncode != 0:
            print(f"[collect] ERROR: Godot exited with code {result.returncode}", file=sys.stderr)
            print(result.stderr, file=sys.stderr)
            sys.exit(1)

        # Parse the exported YAML
        export_path = os.path.join(os.getcwd(), "_node_props_export.yaml")
        if not os.path.exists(export_path):
            print(f"[collect] ERROR: Export file not found at {export_path}", file=sys.stderr)
            sys.exit(1)

        with open(export_path, "r", encoding="utf-8") as f:
            documents = list(yaml.safe_load_all(f))

        count = 0
        for doc in documents:
            if not doc or "class" not in doc:
                continue
            class_name = doc["class"]
            yaml_path = os.path.join(output_dir, f"{class_name}.yaml")
            with open(yaml_path, "w", encoding="utf-8") as f:
                yaml.dump(doc, f, default_flow_style=False, allow_unicode=True)
            count += 1

        print(f"[collect] Generated {count} YAML files in {output_dir}")

    finally:
        # Cleanup
        if os.path.exists(script_path):
            os.remove(script_path)
        export_yaml = os.path.join(os.getcwd(), "_node_props_export.yaml")
        if os.path.exists(export_yaml):
            os.remove(export_yaml)


if __name__ == "__main__":
    main()
