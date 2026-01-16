#!/usr/bin/env python3
from __future__ import annotations

import os
import shutil
import sys
import tempfile
import time

import _common


def _bytes(path: str) -> int:
    try:
        return os.path.getsize(path)
    except Exception:
        return 0


def main() -> None:
    ms = _common.load_merge_module()
    base = os.getcwd()
    cfg = _common.cfg_path(ms)

    prod, merged_dir, outxml, run_db, tor_scale, tree, threads, chunk, tmp, groups = _common.read_cfg(ms, cfg, base)
    project, stage_outdirs = _common.parse_prod_xml(ms, prod)

    gname, kind, stages = _common.first_group(groups)
    nfiles = int(os.environ.get("NFILES", "25"))

    if shutil.which("hadd") is None:
        raise SystemExit("[05] FAIL: hadd not found in PATH")

    inputs = []
    for st in stages:
        od = stage_outdirs.get(st, "")
        if not od:
            continue
        inputs += _common.inputs_from_outdir(ms, od)

    inputs = sorted(set(inputs))
    if not inputs:
        raise SystemExit("[05] FAIL: no inputs found for group")

    subset = inputs[:nfiles] if nfiles > 0 else inputs
    print("[05] group:", gname, "kind:", kind)
    print("[05] inputs total:", len(inputs), "subset:", len(subset))
    print("[05] subrun_tree:", tree)
    print("[05] threads:", threads, "chunk:", chunk, "tmp:", tmp)

    with tempfile.TemporaryDirectory(prefix="merge_bench_") as outdir:
        dest = os.path.join(outdir, f"{gname}_subset.root")

        t0 = time.perf_counter()
        merged_path, cleanup = ms._merge(dest, subset, threads, chunk, tmp)
        t_merge = time.perf_counter() - t0

        try:
            size_mb = _bytes(merged_path) / (1024.0 * 1024.0)
            print(f"[05] merged -> {merged_path}")
            print(f"[05] merge_time: {t_merge:.3f}s  out_size: {size_mb:.1f} MB")

            t1 = time.perf_counter()
            keys, pot_sum = ms._keys_and_pot(merged_path, tree)
            t_pot = time.perf_counter() - t1
            pairs = ms._pairs_from_keys(keys)
            print(f"[05] keys/pot_time: {t_pot:.3f}s  subruns={len(pairs)}  pot_sum={pot_sum:.6g}")

            if not os.path.exists(run_db):
                print("[05] WARN: run_db missing, skipping runinfo query:", run_db)
                return

            t2 = time.perf_counter()
            ea9_sum, tort_raw, ext_raw, matched, ea9c, tortc, extc, ext_by_run = ms._runinfo_sums(run_db, pairs)
            t_db = time.perf_counter() - t2
            print(f"[05] runinfo_time: {t_db:.3f}s  matched={matched}/{len(pairs)}")

            tort_pot = float(tort_raw) * float(tor_scale)

            k = (kind or "").lower()
            if k == "ext":
                if hasattr(ms, "_ext_prescaled"):
                    ext_prescaled = float(ms._ext_prescaled(ext_by_run))
                else:
                    ext_prescaled = float(sum(ext_by_run.values()))
                scale_to_data = (ea9_sum / ext_prescaled) if ext_prescaled > 0 else 0.0
                normalisation = f"{ea9c}/{extc}_prescaled"
            elif k == "data":
                scale_to_data = 1.0
                normalisation = "unity"
            else:
                scale_to_data = (tort_pot / pot_sum) if pot_sum > 0 else 0.0
                normalisation = f"{tortc}*toroid_scale/pot"

            print(f"[05] scale_to_data={scale_to_data:.6g}  normalisation={normalisation}")

            if hasattr(ms, "_write_meta"):
                try:
                    t3 = time.perf_counter()
                    ms._write_meta(
                        merged_path,
                        {
                            "pot_sum": pot_sum,
                            "runinfo_ea9_sum": ea9_sum,
                            "runinfo_tortgt_raw": tort_raw,
                            "runinfo_tortgt_pot": tort_pot,
                            "runinfo_exttrig_raw": ext_raw,
                            "toroid_scale": float(tor_scale),
                            "scale_to_data": float(scale_to_data),
                            "w_norm": float(scale_to_data),
                            "subruns_total": float(len(pairs)),
                            "runinfo_subruns_matched": float(matched),
                        },
                        {
                            "sample_name": gname,
                            "sample_kind": kind,
                            "normalisation": normalisation,
                            "production_xml": prod,
                            "merge_config": cfg,
                            "run_db": run_db,
                            "subrun_tree": tree,
                        },
                    )
                    t_meta = time.perf_counter() - t3
                    print(f"[05] meta_write_time: {t_meta:.3f}s  (PyROOT OK)")
                except Exception as e:
                    print("[05] WARN: meta write failed (PyROOT missing or error):", e)

            files_per_s = (len(subset) / t_merge) if t_merge > 0 else 0.0
            mb_per_s = (size_mb / t_merge) if t_merge > 0 else 0.0
            print(f"[05] merge throughput: {files_per_s:.2f} files/s  {mb_per_s:.1f} MB/s")

            print("[05] PASS")

        finally:
            try:
                cleanup()
            except Exception:
                pass


if __name__ == "__main__":
    try:
        main()
    except Exception as e:
        print("[05] ERROR:", str(e), file=sys.stderr)
        sys.exit(2)
