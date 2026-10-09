"""Headless Ghidra census of heroes3.dynamic: function starts and sizes, call and
data references (build/heroes3-loki/census/). The game's project objects were
compiled without exceptions, so .eh_frame describes only the C++ runtime and
function boundaries come from Ghidra's analysis instead of FDEs."""
from __future__ import annotations

import csv
import os
import shutil
from dataclasses import dataclass
from pathlib import Path

from homm3.loki_game.image import BUILD, stage

CENSUS = BUILD / "census"
PROJECT = BUILD / "ghidra"


@dataclass(frozen=True)
class Function:
    address: int
    size: int
    thunk: bool


def run(heap: str = "3G") -> Path:
    """Import and auto-analyze the staged image (cached project), then dump."""
    binary = stage()
    import pyghidra
    if not pyghidra.started():
        PROJECT.mkdir(parents=True, exist_ok=True)
        launcher = pyghidra.HeadlessPyGhidraLauncher()
        java = shutil.which("java")
        if java:
            launcher.java_home = Path(java).resolve().parent.parent
        launcher.add_vmargs(f"-Xmx{heap}", "-Djava.awt.headless=true",
                            f"-Dapplication.settingsdir={PROJECT / 'settings'}",
                            f"-Dapplication.cachedir={PROJECT / 'cache'}")
        launcher.start()
    from pyghidra.core import _setup_project, _analyze_program
    from ghidra.program.flatapi import FlatProgramAPI
    from ghidra.program.util import GhidraProgramUtilities
    gproject, program = _setup_project(binary_path=str(binary), project_location=str(PROJECT),
                                       project_name="heroes3_loki", nested_project_location=False)
    try:
        if GhidraProgramUtilities.shouldAskToAnalyze(program):
            print("[loki-game] Ghidra auto-analysis of heroes3.dynamic ...", flush=True)
            _analyze_program(FlatProgramAPI(program), program)
            tx = program.startTransaction("mark-analyzed")
            try:
                GhidraProgramUtilities.markProgramAnalyzed(program)
            finally:
                program.endTransaction(tx, True)
            gproject.save(program)
        _dump(program)
    finally:
        gproject.close()
    return CENSUS


def _dump(program) -> None:
    CENSUS.mkdir(parents=True, exist_ok=True)
    listing = program.getListing()
    with (CENSUS / "functions.tsv").open("w") as f, (CENSUS / "calls.tsv").open("w") as c, \
            (CENSUS / "refs.tsv").open("w") as r:
        f.write("entry\tsize\tthunk\n")
        for fn in program.getFunctionManager().getFunctions(True):
            body = fn.getBody()
            entry = fn.getEntryPoint().getOffset()
            f.write(f"{entry:08x}\t{body.getNumAddresses()}\t{int(fn.isThunk())}\n")
            it = listing.getInstructions(body, True)
            while it.hasNext():
                ins = it.next()
                at = ins.getAddress().getOffset()
                for ref in ins.getReferencesFrom():
                    to = ref.getToAddress()
                    if not to.isMemoryAddress():
                        continue
                    kind = ref.getReferenceType()
                    if kind.isCall():
                        c.write(f"{entry:08x}\t{at:08x}\t{to.getOffset():08x}\n")
                    elif kind.isData() or kind.isRead() or kind.isWrite():
                        r.write(f"{entry:08x}\t{at:08x}\t{to.getOffset():08x}\n")


def functions() -> dict[int, Function]:
    path = CENSUS / "functions.tsv"
    if not path.is_file():
        raise RuntimeError("no census; run `homm3 loki-game census`")
    with path.open() as stream:
        return {int(r["entry"], 16): Function(int(r["entry"], 16), int(r["size"]), r["thunk"] == "1")
                for r in csv.DictReader(stream, delimiter="\t")}


def _edges(name: str) -> list[tuple[int, int, int]]:
    with (CENSUS / name).open() as stream:
        return [tuple(int(x, 16) for x in line.split()[:3]) for line in stream]


def calls() -> list[tuple[int, int, int]]:
    return _edges("calls.tsv")


def refs() -> list[tuple[int, int, int]]:
    return _edges("refs.tsv")


def available() -> bool:
    return all((CENSUS / n).is_file() for n in ("functions.tsv", "calls.tsv", "refs.tsv")) \
        and not os.environ.get("HOMM3_LOKI_GAME_RECENSUS")
