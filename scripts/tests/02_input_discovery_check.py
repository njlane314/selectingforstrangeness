#!/usr/bin/env python3
from __future__ import annotations

import os
import sys
import time

import _common


def main() -> None:
    ms = _common.load_merge_module()
    base = os.getcwd()
    cfg = _common.cfg_path(ms)

    prod, merged_dir, outxml, run_db, tor_scale, tree, threads, chunk, tmp, groups = _common.read_cfg(ms, cfg, base)
    project, stage_outdirs = _common.parse_prod_xml(ms, prod)

    print("[02] project:", project)
    print("[02] subrun_tree:", tree)
    print("[02] scanning inputs...")

    t0 = time.perf_counter()
    any_files = 0

    for name, kind, stages in groups:
        per_stage = []
        all_inputs = []
        for st in stages:
            od = stage_outdirs.get(st, "")
            ins = _common.inputs_from_outdir(ms, od) if od else []
            per_stage.append((st, len(ins)))
            all_inputs.extend(ins)

        all_inputs = sorted(set(all_inputs))
        any_files += len(all_inputs)

        print(f"[02] group={name:14s} kind={kind:12s} total_files={len(all_inputs):7d}")
        for st, n in per_stage:
            print(f"      stage={st:20s} files={n:7d}")

        if all_inputs:
            print("      example:", all_inputs[0])
            if len(all_inputs) > 1:
                print("      example:", all_inputs[-1])

    dt = time.perf_counter() - t0
    print(f"[02] scan time: {dt:.3f} s  total_files={any_files}")

    if any_files == 0:
        raise SystemExit("[02] FAIL: no input files discovered in any group")

    print("[02] PASS")


if __name__ == "__main__":
    try:
        main()
    except Exception as e:
        print("[02] ERROR:", str(e), file=sys.stderr)
        sys.exit(2)
