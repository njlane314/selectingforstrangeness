#!/usr/bin/env python3
from __future__ import annotations

import importlib.util
import inspect
import os
from pathlib import Path
from types import ModuleType


def _candidates() -> list[Path]:
    return [
        Path("merge_samples.py"),
        Path("python/merge_samples.py"),
        Path("scripts/merge_samples.py"),
        Path("merge-samples.py"),
    ]


def find_merge_script() -> Path:
    env = os.environ.get("MERGE_SCRIPT", "").strip()
    if env:
        p = Path(env).expanduser()
        if not p.is_absolute():
            p = (Path.cwd() / p).resolve()
        if not p.exists():
            raise RuntimeError(f"MERGE_SCRIPT points to missing file: {p}")
        return p

    for c in _candidates():
        p = (Path.cwd() / c).resolve()
        if p.exists():
            return p

    raise RuntimeError(
        "Could not locate merge script.\n"
        "Set MERGE_SCRIPT=/path/to/your_merge_script.py\n"
        "or place it at ./merge_samples.py"
    )


def load_merge_module() -> ModuleType:
    script = find_merge_script()
    spec = importlib.util.spec_from_file_location("merge_mod", script)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"Failed to import module from {script}")

    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


def cfg_path(mod: ModuleType) -> str:
    env = os.environ.get("MERGE_CONFIG", "").strip()
    if env:
        p = Path(env).expanduser()
        if not p.is_absolute():
            p = (Path.cwd() / p).resolve()
        return str(p)

    for attr in ("DEFAULT_CONFIG", "CONFIG"):
        v = getattr(mod, attr, None)
        if isinstance(v, str) and v.strip():
            p = Path(v.strip())
            if not p.is_absolute():
                p = (Path.cwd() / p).resolve()
            return str(p)

    return str((Path.cwd() / "config/merge-samples.xml").resolve())


def read_cfg(mod: ModuleType, cfg: str, base: str):
    if hasattr(mod, "_read_merge_config"):
        return mod._read_merge_config(cfg, base)
    if hasattr(mod, "_read_cfg"):
        return mod._read_cfg(cfg, base)
    raise RuntimeError("Merge script does not have _read_cfg or _read_merge_config")


def parse_prod_xml(mod: ModuleType, prod_xml: str):
    if hasattr(mod, "_parse_production_xml"):
        return mod._parse_production_xml(prod_xml)
    if hasattr(mod, "_parse_prod_xml"):
        return mod._parse_prod_xml(prod_xml)
    raise RuntimeError("Merge script does not have _parse_prod_xml or _parse_production_xml")


def inputs_from_outdir(mod: ModuleType, outdir: str) -> list[str]:
    fn = getattr(mod, "_inputs_from_outdir", None)
    if fn is None:
        raise RuntimeError("Merge script missing _inputs_from_outdir")

    try:
        return fn(outdir)
    except TypeError:
        sig = inspect.signature(fn)
        if len(sig.parameters) >= 2:
            basenames = getattr(
                mod,
                "DEFAULT_INPUT_BASENAMES",
                getattr(mod, "DEFAULT_INPUT_BASENAME", "nu_selection.root"),
            )
            return fn(outdir, basename=basenames)
        raise


def first_group(groups, prefer: str | None = None):
    if not groups:
        raise RuntimeError("No groups in config")

    env = os.environ.get("GROUP", "").strip()
    want = env or (prefer or "")
    if want:
        for g in groups:
            if g[0] == want:
                return g
        raise RuntimeError(f"GROUP='{want}' not found in config groups")

    return groups[0]
