#!/usr/bin/env python3

from __future__ import annotations

import glob
import os
import re
import shutil
import sqlite3
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET
from functools import lru_cache

import numpy as np
import uproot

DEFAULT_CONFIG = "config/merge-samples.xml"
DEFAULT_RUN_DB = "/exp/uboone/data/uboonebeam/beamdb/run.db"
DEFAULT_INPUT_BASENAME = "nu_selection.root"

try:
    sys.path.append("/exp/uboone/data/uboonebeam/beamdb")
    import confDB  # type: ignore

    _CONFDB = confDB.confDB()
except Exception:
    _CONFDB = None


def _read_text(path: str) -> str:
    with open(path, "r", encoding="utf-8") as f:
        return f.read()


def _abs(path: str, base: str) -> str:
    p = (path or "").strip()
    if not p:
        return ""
    return p if os.path.isabs(p) else os.path.abspath(os.path.join(base, p))


def _as_float(txt: str, default: float) -> float:
    s = (txt or "").strip()
    if not s:
        return default
    return float(s.replace("D", "e").replace("d", "e"))


def _as_int(txt: str, default: int) -> int:
    s = (txt or "").strip()
    if not s:
        return default
    return int(s)


def _expand_entities(xml_txt: str) -> str:
    """
    Expand simple <!ENTITY NAME "value"> substitutions inside a DOCTYPE, then strip the DOCTYPE.
    This matches common production XML patterns used in workflows.
    """
    ents = dict(re.findall(r'<!ENTITY\s+(\w+)\s+"([^"]*)"\s*>', xml_txt))
    done: dict[str, str] = {}

    def res(k: str, stack: tuple[str, ...] = ()) -> str:
        if k in done:
            return done[k]
        if k not in ents:
            return ""
        if k in stack:
            raise RuntimeError("circular entity: " + " -> ".join(stack + (k,)))
        v = re.sub(r"&(\w+);", lambda m: res(m.group(1), stack + (k,)), ents[k])
        done[k] = v
        return v

    for k in list(ents):
        res(k)

    xml_txt = re.sub(r"&(\w+);", lambda m: done.get(m.group(1), m.group(0)), xml_txt)
    return re.sub(r"<!DOCTYPE[\s\S]*?\]>", "", xml_txt, count=1)


def _parse_production_xml(path: str) -> tuple[str, dict[str, str]]:
    """
    Returns: (project_name, {stage_name: outdir})
    """
    root = ET.fromstring(_expand_entities(_read_text(path)))
    proj = root.find("project") or root.find(".//project")
    if proj is None:
        raise RuntimeError("production_xml: could not find <project>")

    project = (proj.attrib.get("name") or "project").strip() or "project"
    outdirs: dict[str, str] = {}

    for st in proj.findall("stage"):
        name = (st.attrib.get("name") or "").strip()
        outdir = (st.findtext("outdir") or "").strip()
        if name and outdir:
            outdirs[name] = outdir

    if not outdirs:
        raise RuntimeError("production_xml: no <stage> outdirs found")

    return project, outdirs


def _read_merge_config(cfg_path: str, base: str):
    """
    Reads XML like:

      <merge_samples>
        <production_xml>...</production_xml>
        <merged_dir>...</merged_dir>
        <output_xml>...</output_xml>
        <run_db>...</run_db>
        <toroid_scale>...</toroid_scale>
        <subrun_tree>...</subrun_tree>
        <hadd_threads>...</hadd_threads>
        <chunk_size>...</chunk_size>
        <tmp_dir>...</tmp_dir>
        <groups>
          <group name="beam" kind="beam">beam_s0,beam_s1</group>
          ...
        </groups>
      </merge_samples>
    """
    r = ET.parse(cfg_path).getroot()

    prod = _abs(r.findtext("production_xml") or "", base)
    merged_dir = _abs(r.findtext("merged_dir") or "merged", base)
    outxml = _abs(r.findtext("output_xml") or "merged-samples.xml", base)

    run_db = _abs(r.findtext("run_db") or DEFAULT_RUN_DB, base)
    tor_scale = _as_float(r.findtext("toroid_scale") or "1e12", 1e12)

    subrun_tree = (r.findtext("subrun_tree") or "nuselection/SubRun").strip()
    threads = _as_int(r.findtext("hadd_threads") or "8", 8)
    chunk = _as_int(r.findtext("chunk_size") or "250", 250)

    tmp_dir = _abs(r.findtext("tmp_dir") or os.environ.get("TMPDIR", "/tmp"), base)
    if not os.path.isdir(tmp_dir):
        tmp_dir = os.environ.get("TMPDIR", "/tmp")

    groups = []
    gs = r.find("groups")
    if gs is not None:
        for g in gs.findall("group"):
            name = (g.attrib.get("name") or "").strip()
            kind = (g.attrib.get("kind") or name).strip()
            stages = [x.strip() for x in (g.text or "").split(",") if x.strip()]
            if name and stages:
                groups.append((name, kind, stages))

    return prod, merged_dir, outxml, run_db, tor_scale, subrun_tree, threads, chunk, tmp_dir, groups


def _inputs_from_outdir(outdir: str, basename: str = DEFAULT_INPUT_BASENAME) -> list[str]:
    """
    MicroBooNE production style: outdir/<jobid>/nu_selection.root
    """
    if not os.path.isdir(outdir):
        return []
    basenames = (basename,)
    if basename == DEFAULT_INPUT_BASENAME:
        basenames = (basename, "nu_selection_data.root")
    out = []
    with os.scandir(outdir) as it:
        for e in it:
            if not e.is_dir():
                continue
            for candidate in basenames:
                p = os.path.join(e.path, candidate)
                if os.path.isfile(p):
                    out.append(p)
                    break
    return sorted(set(out))


def _is_pnfs(p: str) -> bool:
    return p.startswith("/pnfs/")


def _pnfs_to_xrootd(p: str) -> str:
    """
    Prefer local pnfs path if readable; else convert to an xrootd URL.
    """
    if not _is_pnfs(p):
        return p

    try:
        if os.path.isfile(p) and os.access(p, os.R_OK):
            return p
    except Exception:
        pass

    try:
        r = subprocess.run(["pnfs2xrootd", p], check=False, capture_output=True, text=True)
        if r.returncode == 0 and r.stdout.strip():
            return r.stdout.strip()
    except Exception:
        pass

    suffix = p
    if not suffix.startswith("/pnfs/fnal.gov"):
        suffix = "/pnfs/fnal.gov" + p[len("/pnfs") :]
    return f"root://fndca1.fnal.gov:1094{suffix}"


def _uptodate(outp: str, inputs: list[str]) -> bool:
    if not os.path.exists(outp):
        return False
    om = os.path.getmtime(outp)
    return all(os.path.exists(i) and os.path.getmtime(i) <= om for i in inputs)


def _hadd(outp: str, inputs: list[str], threads: int, work: str) -> None:
    if not inputs:
        raise RuntimeError("hadd: no inputs")

    cmd = ["hadd", "-f", "-k"]
    if threads and threads > 1:
        d = os.path.join(work, "hadd_tmp", os.path.basename(outp).replace(".root", ""))
        os.makedirs(d, exist_ok=True)
        cmd += ["-j", str(threads), "-d", d]

    cmd += [outp] + list(inputs)
    subprocess.run(cmd, check=True)


def _merge(dest: str, inputs: list[str], threads: int, chunk: int, tmp: str) -> tuple[str, callable]:
    """
    Merge inputs into dest. If dest is on /pnfs, write locally then copy later (caller decides).
    Returns: (local_merged_path, cleanup_fn)
    """
    inputs = sorted(set(inputs))
    if not inputs:
        raise RuntimeError("merge: no inputs for " + dest)

    os.makedirs(os.path.dirname(dest) or ".", exist_ok=True)

    if _uptodate(dest, inputs):
        return dest, lambda: None

    work = tempfile.mkdtemp(prefix="merge_", dir=tmp if os.path.isdir(tmp) else None)
    cleanup = lambda: shutil.rmtree(work, ignore_errors=True)

    local = os.path.join(work, "merged.root") if _is_pnfs(dest) else dest

    try:
        if len(inputs) == 1:
            shutil.copy2(inputs[0], local)
        else:
            hadd_inputs = [_pnfs_to_xrootd(p) for p in inputs]
            batch = int(chunk) if chunk and chunk > 0 else len(hadd_inputs)

            if len(hadd_inputs) <= batch:
                _hadd(local, hadd_inputs, threads, work)
            else:
                partials = []
                for i in range(0, len(hadd_inputs), batch):
                    part = os.path.join(work, f"part_{i//batch:04d}.root")
                    _hadd(part, hadd_inputs[i : i + batch], threads, work)
                    partials.append(part)
                _hadd(local, partials, threads, work)

        return local, cleanup
    except Exception:
        cleanup()
        raise


def _keys_and_pot(root_path: str, tree_path: str) -> tuple[np.ndarray, float]:
    """
    Read run/subRun/pot from tree and compute:
      - unique (run,subRun) keys
      - pot_sum = sum over unique subruns of max(pot) per subrun
    """
    t = uproot.open(f"{root_path}:{tree_path}")
    run = t["run"].array(library="np").astype(np.int64)
    sub = t["subRun"].array(library="np").astype(np.int64) & np.int64(0xFFFFFFFF)
    pot = t["pot"].array(library="np").astype(np.float64)

    key = (run << np.int64(32)) | sub
    if key.size == 0:
        return key, 0.0

    order = np.argsort(key, kind="mergesort")
    ks = key[order]
    ps = pot[order]

    idx = np.r_[0, np.flatnonzero(ks[1:] != ks[:-1]) + 1]
    pm = np.maximum.reduceat(ps, idx)

    return ks[idx], float(pm.sum())


def _pairs_from_keys(keys: np.ndarray) -> list[tuple[int, int]]:
    r = (keys >> np.int64(32)).astype(np.int64)
    s = (keys & np.int64(0xFFFFFFFF)).astype(np.int64)
    return list(zip(r.tolist(), s.tolist()))


def _pick_col(cols: list[str], candidates: list[str]) -> str:
    m = {c.lower(): c for c in cols}
    for c in candidates:
        if c.lower() in m:
            return m[c.lower()]
    raise RuntimeError("run.db: missing required column(s): " + ",".join(candidates))


def _qident(col: str) -> str:
    # Safe-ish identifier quoting for SQLite
    return '"' + col.replace('"', '""') + '"'


def _runinfo_sums(run_db: str, pairs: list[tuple[int, int]]):
    """
    Query run.db runinfo table for a subset of (run,subrun),
    deduplicate within (run,subrun) by MAX, then sum.
    Also returns ext sum per run (for prescale).
    """
    if not os.path.exists(run_db):
        raise RuntimeError("run.db not found: " + run_db)

    con = sqlite3.connect(run_db)
    con.row_factory = sqlite3.Row
    cur = con.cursor()

    cur.execute("PRAGMA table_info(runinfo);")
    cols = [r[1] for r in cur.fetchall()]
    if not cols:
        con.close()
        raise RuntimeError("run.db: table runinfo not found or has no columns")

    ea9c = _pick_col(cols, ["EA9CNT"])
    tortc = _pick_col(cols, ["tortgt"])
    extc = _pick_col(cols, ["EXTTrig"])

    ea9q, tortq, extq = _qident(ea9c), _qident(tortc), _qident(extc)

    cur.execute("PRAGMA temp_store=MEMORY;")
    cur.execute("CREATE TEMP TABLE pairs(run INTEGER, subrun INTEGER, PRIMARY KEY(run,subrun));")
    cur.executemany("INSERT OR IGNORE INTO pairs(run, subrun) VALUES (?,?);", pairs)

    row = cur.execute(
        f"""
        WITH subset AS (
          SELECT r.run AS run, r.subrun AS subrun,
                 r.{ea9q} AS ea9, r.{tortq} AS tort, r.{extq} AS ext
          FROM runinfo r
          JOIN pairs p ON p.run=r.run AND p.subrun=r.subrun
        ),
        dedup AS (
          SELECT run, subrun, MAX(ea9) AS ea9, MAX(tort) AS tort, MAX(ext) AS ext
          FROM subset
          GROUP BY run, subrun
        )
        SELECT IFNULL(SUM(ea9),0.0) AS ea9_sum,
               IFNULL(SUM(tort),0.0) AS tort_sum,
               IFNULL(SUM(ext),0.0) AS ext_sum,
               COUNT(*) AS matched
        FROM dedup;
        """
    ).fetchone()

    by_run = cur.execute(
        f"""
        WITH subset AS (
          SELECT r.run AS run, r.subrun AS subrun, r.{extq} AS ext
          FROM runinfo r
          JOIN pairs p ON p.run=r.run AND p.subrun=r.subrun
        ),
        dedup AS (
          SELECT run, subrun, MAX(ext) AS ext
          FROM subset
          GROUP BY run, subrun
        )
        SELECT run, IFNULL(SUM(ext),0.0) AS ext_sum
        FROM dedup
        GROUP BY run;
        """
    ).fetchall()

    con.close()

    ext_by_run = {int(r["run"]): float(r["ext_sum"]) for r in by_run}

    return (
        float(row["ea9_sum"]),
        float(row["tort_sum"]),
        float(row["ext_sum"]),
        int(row["matched"]),
        ea9c,
        tortc,
        extc,
        ext_by_run,
    )


@lru_cache(maxsize=4096)
def _prescale(run: int) -> float:
    if _CONFDB is None:
        return 1.0
    try:
        pf = _CONFDB.getAllPrescaleFactors(int(run)) or {}
        for k in ("EXT_NUMIwin_2018May_FEMBeamTriggerAlgo", "EXT_NUMIwin_FEMBeamTriggerAlgo"):
            v = pf.get(k)
            if v is not None and float(v) > 0:
                return float(v)
        for k, v in pf.items():
            if k.startswith("EXT_") and v is not None and float(v) > 0:
                return float(v)
    except Exception:
        return 1.0
    return 1.0


def _ext_prescaled(ext_by_run: dict[int, float]) -> float:
    return float(sum(v * _prescale(r) for r, v in ext_by_run.items()))


def _write_meta(root_path: str, nums: dict[str, float], strs: dict[str, str]) -> None:
    import ROOT  # local import keeps startup lighter

    ROOT.gROOT.SetBatch(True)
    f = ROOT.TFile.Open(root_path, "UPDATE")
    if not f or f.IsZombie():
        raise RuntimeError("could not open ROOT file for update: " + root_path)

    for k, v in nums.items():
        ROOT.TParameter("double")(k, float(v)).Write("", ROOT.TObject.kOverwrite)

    for k, v in strs.items():
        ROOT.TNamed(k, str(v)).Write("", ROOT.TObject.kOverwrite)

    f.Close()


def _write_outxml(path: str, prod: str, project: str, merged_dir: str, run_db: str, tree: str, samples: list[dict]):
    root = ET.Element(
        "merged_samples",
        attrib={
            "production_xml": prod,
            "project": project,
            "merged_dir": merged_dir,
            "run_db": run_db,
            "subrun_tree": tree,
        },
    )
    for s in samples:
        ET.SubElement(root, "sample", attrib={k: str(v) for k, v in s.items()})

    os.makedirs(os.path.dirname(path) or ".", exist_ok=True)
    ET.ElementTree(root).write(path, encoding="utf-8", xml_declaration=True)


def main(cfg_path: str) -> None:
    base = os.getcwd()
    cfg_path = _abs(cfg_path, base)

    if not os.path.exists(cfg_path):
        raise RuntimeError("missing config: " + cfg_path)

    prod, merged_dir, outxml, run_db, tor_scale, tree, threads, chunk, tmp, groups = _read_merge_config(cfg_path, base)

    if not prod or not os.path.exists(prod):
        raise RuntimeError("production_xml not found: " + (prod or "(empty)"))
    if not groups:
        raise RuntimeError("no <groups> defined in " + cfg_path)

    project, stage_outdirs = _parse_production_xml(prod)

    out_proj = os.path.join(merged_dir, project)
    os.makedirs(out_proj, exist_ok=True)

    samples_out: list[dict] = []

    for name, kind, stages in groups:
        out = os.path.join(out_proj, f"{name}.root")

        inputs: list[str] = []
        for st in stages:
            if st not in stage_outdirs:
                raise RuntimeError(f"stage '{st}' not found in production XML")
            basename = (
                "nu_selection_data.root"
                if kind.lower() == "ext"
                else "nu_selection.root"
            )
            inputs += _inputs_from_outdir(stage_outdirs[st], basename=basename)

        inputs = sorted(set(inputs))
        if not inputs:
            print(f"[merge] skip {name}: no inputs found")
            continue

        print(f"[merge] {name} ({kind}): {len(inputs)} files -> {out}")
        merged_local, cleanup = _merge(out, inputs, threads, chunk, tmp)

        try:
            keys, pot_sum = _keys_and_pot(merged_local, tree)
            pairs = _pairs_from_keys(keys)

            ea9_sum, tort_raw, ext_raw, matched, ea9c, tortc, extc, ext_by_run = _runinfo_sums(run_db, pairs)
            tort_pot = float(tort_raw) * float(tor_scale)

            ext_prescaled = float(_ext_prescaled(ext_by_run))
            ext_pot_equiv = 0.0

            k = (kind or "").lower()
            if k == "ext":
                pot_sum = 0.0

            if k == "ext":
                scale_to_data = (ea9_sum / ext_prescaled) if ext_prescaled > 0 else 0.0
                ext_pot_equiv = tort_pot * scale_to_data if ext_prescaled > 0 else 0.0
                normalisation = f"{ea9c}/{extc}_prescaled"
            elif k == "data":
                scale_to_data = 1.0
                normalisation = "unity"
            else:
                scale_to_data = (tort_pot / pot_sum) if pot_sum > 0 else 0.0
                normalisation = f"{tortc}*toroid_scale/pot"

            _write_meta(
                merged_local,
                {
                    "pot_sum": pot_sum,
                    "runinfo_ea9_sum": ea9_sum,
                    "runinfo_tortgt_raw": tort_raw,
                    "runinfo_tortgt_pot": tort_pot,
                    "runinfo_exttrig_raw": ext_raw,
                    "exttrig_prescaled": ext_prescaled,
                    "ext_pot_equiv": ext_pot_equiv,
                    "toroid_scale": tor_scale,
                    "scale_to_data": scale_to_data,
                    "w_norm": scale_to_data,  # alias for downstream code that expects a single global weight
                    "subruns_total": float(len(pairs)),
                    "runinfo_subruns_matched": float(matched),
                },
                {
                    "sample_name": name,
                    "sample_kind": kind,
                    "normalisation": normalisation,
                    "production_xml": prod,
                    "merge_config": cfg_path,
                    "run_db": run_db,
                    "ea9_column": ea9c,
                    "tortgt_column": tortc,
                    "exttrig_column": extc,
                    "subrun_tree": tree,
                    "prescale_applied": "yes" if _CONFDB is not None else "no",
                },
            )

            if _is_pnfs(out) and merged_local != out:
                shutil.copy2(merged_local, out)

            if matched != len(pairs):
                print(f"[merge] WARN {name}: matched {matched}/{len(pairs)} subruns in runinfo")

            samples_out.append(
                {
                    "name": name,
                    "kind": kind,
                    "file": out,
                    "subruns_total": len(pairs),
                    "pot_sum": f"{pot_sum:.17g}",
                    "runinfo_ea9_sum": f"{ea9_sum:.17g}",
                    "runinfo_tortgt_pot": f"{tort_pot:.17g}",
                    "scale_to_data": f"{scale_to_data:.17g}",
                    "normalisation": normalisation,
                    "exttrig_raw": f"{ext_raw:.17g}",
                    "exttrig_prescaled": f"{ext_prescaled:.17g}",
                    "ext_pot_equiv": f"{ext_pot_equiv:.17g}",
                }
            )

        finally:
            cleanup()

    _write_outxml(outxml, prod, project, merged_dir, run_db, tree, samples_out)
    print(f"[merge] wrote {outxml}")


if __name__ == "__main__":
    cfg = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_CONFIG
    try:
        main(cfg)
    except Exception as e:
        print(str(e), file=sys.stderr)
        sys.exit(2)
