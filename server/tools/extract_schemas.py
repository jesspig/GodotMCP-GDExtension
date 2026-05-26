#!/usr/bin/env python3
"""Tool for maintaining registry.py - the authoritative source for tool schemas.

Usage:
    python tools/extract_schemas.py              # print tools to stdout
    python tools/extract_schemas.py --validate    # validate registry
"""

import ast
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent.parent
REGISTRY_PY = REPO_ROOT / "server" / "src" / "godot_mcp_server" / "registry.py"


def load_tools_from_registry() -> list:
    """Load tools from registry.py by parsing the _TOOLS list."""
    source = REGISTRY_PY.read_text(encoding="utf-8")
    tree = ast.parse(source)
    
    for node in ast.walk(tree):
        if isinstance(node, ast.Assign):
            for target in node.targets:
                if isinstance(target, ast.Name) and target.id == "_TOOLS":
                    if isinstance(node.value, ast.List):
                        tools = []
                        for elt in node.value.elts:
                            if isinstance(elt, ast.Tuple):
                                if len(elt.elts) == 3:
                                    name = ast.literal_eval(elt.elts[0])
                                    desc = ast.literal_eval(elt.elts[1])
                                    schema = ast.literal_eval(elt.elts[2])
                                    tools.append({
                                        "name": name,
                                        "description": desc,
                                        "input_schema": schema
                                    })
                        return tools
    return []


def main():
    tools = load_tools_from_registry()
    names = {t["name"] for t in tools}
    
    print(f"Found {len(tools)} tools ({len(names)} unique)", file=sys.stderr)
    
    if "--validate" in sys.argv:
        duplicates = [n for n in names if sum(1 for t in tools if t["name"] == n) > 1]
        if duplicates:
            print(f"ERROR: Duplicate tool names found: {duplicates}", file=sys.stderr)
            sys.exit(1)
        print("Validation passed!", file=sys.stderr)
    else:
        for t in tools:
            print(f"{t['name']}: {t['description']}")


if __name__ == "__main__":
    main()
