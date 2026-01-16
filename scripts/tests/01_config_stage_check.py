#!/usr/bin/env python3
from __future__ import annotations

import os
import sys

import _common


def main() -> None:
    ms = _common.load_merge_module()
    base = os.getcwd()
    cfg = _common.cfg_path(ms)

    if not os.path.exists(cfg):
        raise SystemExit(f"[01] config missing: {cfg}")

    prod, merged_dir, outxml, run_db, tor_scale, tree, threads, chunk, tmp, groups = _common.read_cfg(ms, cfg, base)

    print("[01] cfg:", cfg)
    print("[01] production_xml:", prod, "exists:", os.path.exists(prod))
    print("[01] merged_dir:", merged_dir)
    print("[01] output_xml:", outxml)
    print("[01] run_db:", run_db, "exists:", os.path.exists(run_db))
    print("[01] toroid_scale:", tor_scale)
    print("[01] subrun_tree:", tree)
    print("[01] hadd_threads:", threads, "chunk_size:", chunk, "tmp_dir:", tmp)
    print("[01] groups:", len(groups))
    for name, kind, stages in groups:
        print(f"  - {name} kind={kind} stages={stages}")

    if not os.path.exists(prod):
        raise SystemExit("[01] FAIL: production_xml not found")

    project, stage_outdirs = _common.parse_prod_xml(ms, prod)
    print("[01] project:", project)
    print("[01] production stages:", len(stage_outdirs))

    used_stages = sorted({st for _, _, sts in groups for st in sts})
    missing = [st for st in used_stages if st not in stage_outdirs]
    if missing:
        print("[01] FAIL: stages referenced by groups but not in production XML:")
        for st in missing:
            print("  *", st)
        raise SystemExit(2)

    strict = os.environ.get("STRICT_STAGE_DIRS", "").strip() == "1"

    bad_dirs = []
    for st in used_stages:
        od = stage_outdirs[st]
        ok = os.path.isdir(od)
        print(f"[01] stage {st}: outdir={od} exists={ok}")
        if not ok:
            bad_dirs.append((st, od))

    if bad_dirs:
        print("[01] WARN: some stage outdirs do not exist (likely stages not produced):")
        for st, od in bad_dirs:
            print(f"  * {st}: {od}")
        if strict:
            raise SystemExit(2)

    print("[01] PASS")


if __name__ == "__main__":
    try:
        main()
    except Exception as e:
        print("[01] ERROR:", str(e), file=sys.stderr)
        sys.exit(2)
