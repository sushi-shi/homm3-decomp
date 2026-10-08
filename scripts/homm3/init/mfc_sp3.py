"""homm3.init.mfc_sp3 - stage the VC6 SP3 MFC overlay for the editor images.

The shared toolchain release carries the RTM MFC 4.2 (`mfc/LIB/NAFXCW.LIB`,
every member stamped C++ 8168). The GOG editors link the SP3 library (every
member stamped 8447; twelve SP3-only bodies such as `CString::FormatV` and
`CWnd::CreateDlgIndirect` occur in both editors), and their sources compile
against SP3's changed MFC headers (`afxwin1.inl`, `afxbld_.h`, ...).

The overlay comes from the same pinned TechNet disc the toolchain release
uses (TNSB9908.iso, VSTUDIO/SP3/Vs6sp3_1.cab, `vc98/mfc`). Every file is
hash-checked against config/project.toml `[toolchain.mfc_sp3.files]` and
copied to build/mfc-sp3/; an image's units put build/mfc-sp3/include ahead of
the toolchain's mfc/INCLUDE.

    homm3 --image h3maped init --mfc-sp3 DIR    # DIR = the extracted vc98/mfc
"""
from __future__ import annotations

import hashlib
import shutil
from pathlib import Path

from homm3.core import common

DESTINATION = common.HOMM3_DIR / "build/mfc-sp3"


def pins() -> dict[str, str]:
    from homm3.core.project import Project
    spec = Project(common.HOMM3_DIR).specification
    return dict(spec.get("toolchain", {}).get("mfc_sp3", {}).get("files", {}))


def staged() -> bool:
    return all((DESTINATION / rel).is_file() for rel in pins())


def stage(source: str | Path) -> Path:
    """Verify every pinned file under `source` and copy it into build/mfc-sp3."""
    source = Path(source).expanduser().resolve()
    files = pins()
    if not files:
        raise ValueError("config/project.toml has no [toolchain.mfc_sp3.files] pins")
    for rel, digest in files.items():
        path = source / rel
        if not path.is_file():
            raise ValueError(f"SP3 MFC overlay: {path} is missing")
        actual = hashlib.sha256(path.read_bytes()).hexdigest()
        if actual != digest:
            raise ValueError(f"SP3 MFC overlay: {path} sha256 {actual} != pinned {digest}")
    for rel in files:
        target = DESTINATION / rel
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(source / rel, target)
    return DESTINATION
