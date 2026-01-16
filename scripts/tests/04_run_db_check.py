#!/usr/bin/env python3
from __future__ import annotations

import os
import sys
import time

import numpy as np
import uproot

import _common


def main() -> None:
    ms = _common.load_merge_module()
    base = os.getcwd()
    cfg = _common.cfg_path(ms)

    prod, merged_dir, outxml, run_db, tor_scale, tree, threads, chunk, tmp, groups = _common.read_cfg(ms, cfg, base)
    project, stage_outdirs = _common.parse_prod_xml(ms, prod)

    gname, kind, stages = _common.first_group(groups)
    print("[04] using group:", gname, "kind:", kind)

    sample_file = ""
    for st in stages:
        od = stage_outdirs.get(st, "")
        if not od:
            continue
        ins = _common.inputs_from_outdir(ms, od)
        if ins:
            sample_file = ins[0]
            break

    if not sample_file:
        raise SystemExit("[04] FAIL: could not find any input file for selected group")

    print("[04] sample file:", sample_file)
    print("[04] subrun_tree:", tree)
    print("[04] run_db:", run_db, "exists:", os.path.exists(run_db))

    if not os.path.exists(run_db):
        raise SystemExit("[04] FAIL: run_db does not exist")

    nmax = int(os.environ.get("NENTRIES", "20000"))

    t0 = time.perf_counter()
    t = uproot.open(f"{sample_file}:{tree}")
    run = t["run"].array(library="np", entry_stop=nmax).astype(np.int64)
    sub = (t["subRun"].array(library="np", entry_stop=nmax).astype(np.int64) & np.int64(0xFFFFFFFF))

    keys = (run << np.int64(32)) | sub
    keys = np.unique(keys)
    pairs = list(zip((keys >> np.int64(32)).astype(np.int64).tolist(),
                     (keys & np.int64(0xFFFFFFFF)).astype(np.int64).tolist()))
    t_read = time.perf_counter() - t0

    print(f"[04] extracted unique subruns: {len(pairs)} (from up to {nmax} entries) in {t_read:.3f}s")

    t1 = time.perf_counter()
    ea9_sum, tort_sum, ext_sum, matched, ea9c, tortc, extc, ext_by_run = ms._runinfo_sums(run_db, pairs)
    t_db = time.perf_counter() - t1

    frac = (matched / len(pairs)) if pairs else 0.0
    print("[04] columns:", {"EA9": ea9c, "tortgt": tortc, "EXTTrig": extc})
    print(f"[04] matched: {matched}/{len(pairs)}  ({frac:.3%})  query_time={t_db:.3f}s")
    print(f"[04] sums: ea9_sum={ea9_sum:.6g}  tort_sum={tort_sum:.6g}  ext_sum={ext_sum:.6g}")

    if hasattr(ms, "_ext_prescaled"):
        try:
            ext_prescaled = float(ms._ext_prescaled(ext_by_run))
            print(f"[04] ext_prescaled: {ext_prescaled:.6g}  (confDB={'yes' if getattr(ms,'_CONFDB',None) else 'no'})")
        except Exception as e:
            print("[04] WARN: _ext_prescaled failed:", e)

    if matched == 0:
        raise SystemExit("[04] FAIL: matched==0 (DB join not working for these subruns)")

    print("[04] PASS")


if __name__ == "__main__":
    try:
        main()
    except Exception as e:
        print("[04] ERROR:", str(e), file=sys.stderr)
        sys.exit(2)
