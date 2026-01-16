#!/usr/bin/env python3
from __future__ import annotations

import os
import sys

import numpy as np
import uproot

import _common


def main() -> None:
    ms = _common.load_merge_module()
    base = os.getcwd()
    cfg = _common.cfg_path(ms)

    prod, merged_dir, outxml, run_db, tor_scale, tree, threads, chunk, tmp, groups = _common.read_cfg(ms, cfg, base)
    project, stage_outdirs = _common.parse_prod_xml(ms, prod)

    max_groups = int(os.environ.get("MAX_GROUPS", "4"))
    max_per_group = int(os.environ.get("MAX_FILES_PER_GROUP", "2"))

    print("[03] checking tree/branches:")
    print("[03] subrun_tree:", tree)
    print("[03] MAX_GROUPS:", max_groups, "MAX_FILES_PER_GROUP:", max_per_group)

    checked = 0
    failed = 0

    for gi, (name, kind, stages) in enumerate(groups):
        if gi >= max_groups:
            break

        files = []
        for st in stages:
            od = stage_outdirs.get(st, "")
            if not od:
                continue
            ins = _common.inputs_from_outdir(ms, od)
            files.extend(ins)
            if len(files) >= max_per_group:
                break

        files = sorted(set(files))[:max_per_group]
        if not files:
            print(f"[03] WARN group={name}: no files to check")
            continue

        for fp in files:
            checked += 1
            try:
                t = uproot.open(f"{fp}:{tree}")
                branches = set(t.keys())

                missing = [b for b in ("run", "subRun", "pot") if b not in branches]
                if missing:
                    failed += 1
                    print(f"[03] FAIL file={fp}")
                    print(f"     missing branches: {missing}")
                    some = sorted(list(branches))[:30]
                    print(f"     first branches: {some}")
                    continue

                run = t["run"].array(library="np", entry_stop=10)
                sub = t["subRun"].array(library="np", entry_stop=10)
                pot = t["pot"].array(library="np", entry_stop=10)

                print(f"[03] OK  file={fp}")
                print(f"     run dtype={run.dtype}  subRun dtype={sub.dtype}  pot dtype={pot.dtype}  n={len(pot)}")

            except Exception as e:
                failed += 1
                print(f"[03] FAIL file={fp}  error={e}")

    print(f"[03] checked={checked} failed={failed}")
    if checked == 0:
        raise SystemExit("[03] FAIL: checked 0 files (nothing to validate)")
    if failed:
        raise SystemExit(2)

    print("[03] PASS")


if __name__ == "__main__":
    try:
        main()
    except Exception as e:
        print("[03] ERROR:", str(e), file=sys.stderr)
        sys.exit(2)
