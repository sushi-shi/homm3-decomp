"""Load sibling generators used by existing source-family experiments."""

import importlib.util
from pathlib import Path


def generator(name):
    path = Path(__file__).resolve().parent / name
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module
