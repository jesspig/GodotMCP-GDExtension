"""纯 Python zip 打包，不依赖 CMake CPack。"""

import os
import zipfile
from pathlib import Path

from scripts._project import PROJECT_ROOT


def create_addons_zip(output_path: str | Path, source_dir: Path | None = None) -> None:
    """将 source_dir（默认为 example/）打包为 addons.zip。

    zip 内路径以 addons/ 开头，可直接解压到 Godot 项目根目录。
    """
    if source_dir is None:
        source_dir = PROJECT_ROOT / "example"

    root = source_dir
    if not root.is_dir():
        print(f"[ERROR] Source directory not found: {root}", flush=True)
        return

    count = 0
    with zipfile.ZipFile(str(output_path), "w", zipfile.ZIP_DEFLATED) as zf:
        for fpath in sorted(root.rglob("*")):
            if fpath.is_file():
                arcname = fpath.relative_to(root.parent)
                zf.write(fpath, arcname)
                count += 1

    print(f"\n[PACKAGE] {output_path}  ({count} files, "
          f"{os.path.getsize(output_path) / 1024:.0f} KB)", flush=True)
