"""Project paths shared by the Gruntz-derived model and data pipeline."""
from homm3.core.common import HOMM3_DIR

ROOT = HOMM3_DIR
BUILD = ROOT / "build"
CONFIG = ROOT / "config"
RETAIL = CONFIG / "retail"
SRC = ROOT / "src"
INCLUDE = ROOT / "include"


def retail_exe():
    from homm3.core import inputs
    return inputs.stage_executable(inputs.RETAIL)

REPO = ROOT

def msvc_dir():
    from homm3.core.project import Project
    return Project(ROOT).toolchain
