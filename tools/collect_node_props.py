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
    var STORAGE_OR_EDITOR = 2 | 4
    var GROUP_FLAGS = (1<<12) | (1<<13)

    # ── Pass 1: Node subclasses ──
    var node_count = 0
    for cname in all_classes:
        if not ClassDB.is_parent_class(cname, "Node"):
            continue

        var inherits = ClassDB.get_parent_class(cname)
        var own_props = _collect_props(cname, STORAGE_OR_EDITOR, GROUP_FLAGS)

        file.store_line("---")
        file.store_line("type: node")
        file.store_line("class: " + cname)
        file.store_line("inherits: " + inherits)
        file.store_line("properties:")
        for prop in own_props:
            file.store_line("  - name: " + prop["name"])
            file.store_line("    type: " + str(prop["type"]))
            file.store_line("    type_name: " + prop["type_name"])
            if prop.get("class_name", "") != "":
                file.store_line("    class_name: " + prop["class_name"])

        node_count += 1

    # ── Pass 2: Resource subclasses ──
    var res_count = 0
    for cname in all_classes:
        if not ClassDB.is_parent_class(cname, "Resource"):
            continue

        var inherits = ClassDB.get_parent_class(cname)
        var own_props = _collect_props(cname, STORAGE_OR_EDITOR, GROUP_FLAGS)

        file.store_line("---")
        file.store_line("type: resource")
        file.store_line("class: " + cname)
        file.store_line("inherits: " + inherits)
        file.store_line("properties:")
        for prop in own_props:
            file.store_line("  - name: " + prop["name"])
            file.store_line("    type: " + str(prop["type"]))
            file.store_line("    type_name: " + prop["type_name"])
            if prop.get("class_name", "") != "":
                file.store_line("    class_name: " + prop["class_name"])

        res_count += 1

    file.close()
    print("Exported " + str(node_count) + " node types, " + str(res_count) + " resource types")
    quit()

func _collect_props(cname, STORAGE_OR_EDITOR, GROUP_FLAGS):
    var prop_list = ClassDB.class_get_property_list(cname, true)
    var result = []
    for prop in prop_list:
        var usage = prop.get("usage", 0)
        if (usage & STORAGE_OR_EDITOR) == 0:
            continue
        if (usage & GROUP_FLAGS) != 0:
            continue
        var tn = _variant_names.get(prop["type"], "unknown")
        var entry = {
            "name": prop["name"],
            "type": prop["type"],
            "type_name": tn
        }
        # For Object-type properties, capture the resource class name
        if prop["type"] == 24:
            entry["class_name"] = prop.get("class_name", "")
        result.append(entry)
    return result
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
        help="Output directory for node YAML files",
    )
    parser.add_argument(
        "--resource-output-dir",
        default=None,
        help="Output directory for resource YAML files",
    )
    args = parser.parse_args()

    script_dir = os.path.dirname(os.path.abspath(__file__))

    node_out_dir = args.output_dir
    if not node_out_dir:
        node_out_dir = os.path.join(script_dir, "..", "extensions", "src", "built_in", "tools", "node_props", "db")

    res_out_dir = args.resource_output_dir
    if not res_out_dir:
        res_out_dir = os.path.join(script_dir, "..", "extensions", "src", "built_in", "tools", "node_resource", "db")

    os.makedirs(node_out_dir, exist_ok=True)
    os.makedirs(res_out_dir, exist_ok=True)

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

        def load_existing(dir_path):
            existing = {}
            if os.path.isdir(dir_path):
                for fname in os.listdir(dir_path):
                    if not fname.endswith(".yaml"):
                        continue
                    cls = fname[:-5]
                    fpath = os.path.join(dir_path, fname)
                    with open(fpath, encoding="utf-8") as f:
                        data = yaml.safe_load(f)
                    if data:
                        existing[cls] = data
            return existing

        existing_nodes = load_existing(node_out_dir)
        existing_resources = load_existing(res_out_dir)

        node_count = 0
        res_count = 0

        for doc in documents:
            if not doc or "class" not in doc:
                continue
            cname = doc["class"]
            doc_type = doc.get("type", "node")

            # 合并：新数据（属性） + 现有元数据（description/aliases）
            merged = {k: v for k, v in doc.items() if k in ("class", "inherits", "properties")}
            if merged.get("properties") is None:
                merged["properties"] = []

            if doc_type == "resource":
                existing = existing_resources
                out_dir = res_out_dir
            else:
                existing = existing_nodes
                out_dir = node_out_dir

            if cname in existing:
                merged["description"] = existing[cname].get("description", "")
                merged["aliases"] = existing[cname].get("aliases", [])
            else:
                merged["description"] = ""
                merged["aliases"] = []

            yaml_path = os.path.join(out_dir, f"{cname}.yaml")
            with open(yaml_path, "w", encoding="utf-8") as f:
                yaml.dump(merged, f, default_flow_style=False, allow_unicode=True, sort_keys=False)

            if doc_type == "resource":
                res_count += 1
            else:
                node_count += 1

        print(f"[collect] Generated {node_count} node YAML files in {node_out_dir}")
        print(f"[collect] Generated {res_count} resource YAML files in {res_out_dir}")

    finally:
        if os.path.exists(script_path):
            os.remove(script_path)
        export_yaml = os.path.join(example_dir, "_node_props_export.yaml")
        if os.path.exists(export_yaml):
            os.remove(export_yaml)


if __name__ == "__main__":
    main()
