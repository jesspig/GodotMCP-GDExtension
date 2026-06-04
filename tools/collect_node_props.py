#!/usr/bin/env python3
"""
collect_node_props.py — 从 Godot 运行时提取节点属性数据

通过 Godot 的 --headless 模式运行 GDScript，从 ClassDB 中提取所有 Node 派生类型的
独有检查器属性，生成 YAML 文件到 extensions/src/built_in/tools/node_props/db/。

用法:
    uv run python tools/collect_node_props.py --godot /path/to/godot

注意: 需要 Godot 4.6+ 可执行文件。
"""

import argparse
import os
import subprocess
import sys

try:
    import yaml
except ImportError:
    print("[collect] ERROR: pyyaml not installed. Run: uv add pyyaml", file=sys.stderr)
    sys.exit(1)


GDSCRIPT_TEMPLATE = """extends SceneTree

var _variant_names = {}

func _init():
    _variant_names = {
        0: "NIL", 1: "bool", 2: "int", 3: "float", 4: "String",
        5: "Vector2", 6: "Vector2i", 7: "Rect2", 8: "Rect2i",
        9: "Vector3", 10: "Vector3i", 11: "Transform2D", 12: "Vector4",
        13: "Vector4i", 14: "Plane", 15: "Quaternion", 16: "AABB",
        17: "Basis", 18: "Transform3D", 19: "Projection", 20: "Color",
        21: "StringName", 22: "NodePath", 23: "RID", 24: "Object",
        25: "Callable", 26: "Signal", 27: "Dictionary", 28: "Array",
        29: "PackedByteArray", 30: "PackedInt32Array", 31: "PackedInt64Array",
        32: "PackedFloat32Array", 33: "PackedFloat64Array", 34: "PackedStringArray",
        35: "PackedVector2Array", 36: "PackedVector3Array", 37: "PackedColorArray",
        38: "PackedVector4Array",
    }

    var file = FileAccess.open("res://_node_props_export.yaml", FileAccess.WRITE)
    if file == null:
        print("ERROR: cannot open output file")
        quit(1)

    var all_classes = ClassDB.get_class_list()
    all_classes.sort()
    var processed = 0

    for cname in all_classes:
        if not ClassDB.is_parent_class(cname, "Node"):
            continue

        var inherits = ClassDB.get_parent_class(cname)

        var own_props = []
        var prop_list = ClassDB.class_get_property_list(cname, true)
        var STORAGE_OR_EDITOR = 2 | 4
        var GROUP_FLAGS = (1<<12) | (1<<13)

        for prop in prop_list:
            var usage = prop.get("usage", 0)
            if (usage & STORAGE_OR_EDITOR) == 0:
                continue
            if (usage & GROUP_FLAGS) != 0:
                continue
            var tn = _variant_names.get(prop["type"], "unknown")
            own_props.append({
                "name": prop["name"],
                "type": prop["type"],
                "type_name": tn
            })

        file.store_line("---")
        file.store_line("class: " + cname)
        file.store_line("inherits: " + inherits)
        file.store_line("properties:")
        for prop in own_props:
            file.store_line("  - name: " + prop["name"])
            file.store_line("    type: " + str(prop["type"]))
            file.store_line("    type_name: " + prop["type_name"])

        processed += 1

    file.close()
    print("Exported " + str(processed) + " node types")
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
        help="Output directory for YAML files",
    )
    args = parser.parse_args()

    output_dir = args.output_dir
    if not output_dir:
        script_dir = os.path.dirname(os.path.abspath(__file__))
        output_dir = os.path.join(script_dir, "..", "extensions", "src", "built_in", "tools", "node_props", "db")

    os.makedirs(output_dir, exist_ok=True)

    # 找 example 项目目录（用于 --path 参数，Godot 需要 project.godot）
    example_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "example")
    example_dir = os.path.normpath(example_dir)
    if not os.path.isfile(os.path.join(example_dir, "project.godot")):
        print(f"[collect] ERROR: example/project.godot not found at {example_dir}", file=sys.stderr)
        sys.exit(1)

    script_path = os.path.join(example_dir, "_collect_props.gd")
    with open(script_path, "w", encoding="utf-8") as f:
        f.write(GDSCRIPT_TEMPLATE)

    try:
        print(f"[collect] Running Godot to extract property data...")
        result = subprocess.run(
            [args.godot, "--headless", "--path", example_dir, "--script", "_collect_props.gd"],
            capture_output=True,
            text=True,
            timeout=120,
        )

        if result.returncode != 0:
            print(f"[collect] ERROR: Godot exited with code {result.returncode}", file=sys.stderr)
            print(result.stderr, file=sys.stderr)
            sys.exit(1)

        export_path = os.path.join(example_dir, "_node_props_export.yaml")
        if not os.path.exists(export_path):
            print(f"[collect] ERROR: Export file not found at {export_path}", file=sys.stderr)
            sys.exit(1)

        with open(export_path, "r", encoding="utf-8") as f:
            documents = list(yaml.safe_load_all(f))

        # 加载现有 YAML 文件，保留 description/aliases（ClassDB headless 模式无法提供）
        existing = {}
        if os.path.isdir(output_dir):
            for fname in os.listdir(output_dir):
                if not fname.endswith(".yaml"):
                    continue
                cls = fname[:-5]
                with open(os.path.join(output_dir, fname), encoding="utf-8") as f:
                    existing_data = yaml.safe_load(f)
                if existing_data:
                    existing[cls] = existing_data

        count = 0
        for doc in documents:
            if not doc or "class" not in doc:
                continue
            cname = doc["class"]

            # 合并：新数据（属性） + 现有元数据（description/aliases）
            merged = {k: v for k, v in doc.items() if k in ("class", "inherits", "properties")}
            if merged.get("properties") is None:
                merged["properties"] = []
            if cname in existing:
                merged["description"] = existing[cname].get("description", "")
                merged["aliases"] = existing[cname].get("aliases", [])
            else:
                merged["description"] = ""
                merged["aliases"] = []

            yaml_path = os.path.join(output_dir, f"{cname}.yaml")
            with open(yaml_path, "w", encoding="utf-8") as f:
                yaml.dump(merged, f, default_flow_style=False, allow_unicode=True, sort_keys=False)
            count += 1

        print(f"[collect] Generated {count} YAML files in {output_dir}")

    finally:
        if os.path.exists(script_path):
            os.remove(script_path)
        export_yaml = os.path.join(example_dir, "_node_props_export.yaml")
        if os.path.exists(export_yaml):
            os.remove(export_yaml)


if __name__ == "__main__":
    main()
