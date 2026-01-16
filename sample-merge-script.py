import glob
import os
import os.path
import pty
import re
import select
import shutil
import sqlite3
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET
from functools import lru_cache

import numpy as np
import uproot

CONFIG = "config/merge-samples.xml"
DEFAULT_RUN_DB = "/exp/uboone/data/uboonebeam/beamdb/run.db"

try:
    sys.path.append("/exp/uboone/data/uboonebeam/beamdb")
    import confDB
    _CONFDB = confDB.confDB()
except Exception:
    _CONFDB = None

def _read(p):
    with open(p, "r", encoding="utf-8") as f:
        return f.read()

def _abs(p, base):
    p = (p or "").strip()
    return p if (p and os.path.isabs(p)) else (os.path.abspath(os.path.join(base, p)) if p else "")

def _expand_entities(txt):
    ents = dict(re.findall(r'<!ENTITY\s+(\w+)\s+"([^"]*)"\s*>', txt))
    done = {}
    def res(k, stack=()):
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
    txt = re.sub(r"&(\w+);", lambda m: done.get(m.group(1), m.group(0)), txt)
    return re.sub(r"<!DOCTYPE[\s\S]*?\]>", "", txt, count=1)

def _parse_prod_xml(path):
    root = ET.fromstring(_expand_entities(_read(path)))
    proj = root.find("project") or root.find(".//project")
    if proj is None:
        raise RuntimeError("could not find <project> in production XML")
    name = (proj.attrib.get("name") or "project").strip()
    outdirs = {}
    for st in proj.findall("stage"):
        s = (st.attrib.get("name") or "").strip()
        d = (st.findtext("outdir") or "").strip()
        if s and d:
            outdirs[s] = d
    if not outdirs:
        raise RuntimeError("no <stage> outdirs found in production XML")
    return name, outdirs

def _classify(stage):
    s = (stage or "").lower()
    if s.startswith("ext"):
        return "ext"
    if s.startswith("dirt"):
        return "dirt"
    if "strange" in s:
        return "strangeness"
    if s.startswith("data"):
        return "data"
    return "beam"

def _find_prod_xml(cwd):
    cands = [p for p in sorted(glob.glob(os.path.join(cwd, "*.xml"))) if os.path.basename(p) not in {CONFIG, "merged-samples.xml"}]
    for p in cands:
        h = _read(p)[:20000]
        if "<stage" in h and "<project" in h:
            return p
    return cands[0] if cands else ""

def _write_cfg(cfg, prod, outdirs):
    kinds = {}
    for s in sorted(outdirs):
        kinds.setdefault(_classify(s), []).append(s)
    root = ET.Element("merge_samples")
    ET.SubElement(root, "production_xml").text = os.path.basename(prod)
    ET.SubElement(root, "merged_dir").text = "merged"
    ET.SubElement(root, "output_xml").text = "merged-samples.xml"
    ET.SubElement(root, "run_db").text = DEFAULT_RUN_DB
    ET.SubElement(root, "toroid_scale").text = "1e12"
    ET.SubElement(root, "subrun_tree").text = "nuselection/SubRun"
    ET.SubElement(root, "hadd_threads").text = "8"
    ET.SubElement(root, "chunk_size").text = "250"
    ET.SubElement(root, "tmp_dir").text = os.environ.get("TMPDIR", "/tmp")
    gs = ET.SubElement(root, "groups")
    order = [k for k in ("beam", "dirt", "ext", "strangeness", "data") if k in kinds] + [k for k in sorted(kinds) if k not in {"beam","dirt","ext","strangeness","data"}]
    for k in order:
        g = ET.SubElement(gs, "group", attrib={"name": k, "kind": k})
        g.text = ",".join(kinds[k])
    ET.SubElement(root, "note").text = "This configuration uses run.db only (runinfo: EA9CNT, tortgt, EXTTrig)."
    ET.ElementTree(root).write(cfg, encoding="utf-8", xml_declaration=True)

def _read_cfg(cfg, base):
    r = ET.parse(cfg).getroot()
    prod = _abs(r.findtext("production_xml"), base)
    merged = _abs(r.findtext("merged_dir") or "merged", base)
    outxml = _abs(r.findtext("output_xml") or "merged-samples.xml", base)
    run_db = _abs(r.findtext("run_db") or DEFAULT_RUN_DB, base)
    tor_scale = float((r.findtext("toroid_scale") or "1e12").strip().replace("D", "e").replace("d", "e"))
    tree = (r.findtext("subrun_tree") or "nuselection/SubRun").strip()
    threads = int((r.findtext("hadd_threads") or "8").strip())
    chunk = int((r.findtext("chunk_size") or "250").strip())
    tmp = _abs(r.findtext("tmp_dir") or os.environ.get("TMPDIR", "/tmp"), base)
    groups = []
    gs = r.find("groups")
    if gs is not None:
        for g in gs.findall("group"):
            name = (g.attrib.get("name") or "").strip()
            kind = (g.attrib.get("kind") or name).strip()
            stages = [x.strip() for x in (g.text or "").split(",") if x.strip()]
            if name and stages:
                groups.append((name, kind, stages))
    return prod, merged, outxml, run_db, tor_scale, tree, threads, chunk, tmp, groups

def _run(cmd):
    p = subprocess.Popen(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        bufsize=0,
    )

    captured = bytearray()
    assert p.stdout is not None
    try:
        for data in iter(lambda: p.stdout.read(8192), b""):
            if not data:
                break
            sys.stdout.buffer.write(data)
            sys.stdout.buffer.flush()
            captured += data
    finally:
        p.stdout.close()

    rc = p.wait()
    if rc != 0:
        raise RuntimeError(
            "command failed: " + " ".join(cmd) + "\n" + captured.decode("utf-8", "replace")
        )

def _is_pnfs(p):
    return p.startswith("/pnfs/")

def _uptodate(outp, inputs):
    if not os.path.exists(outp):
        return False
    om = os.path.getmtime(outp)
    return all(os.path.exists(i) and os.path.getmtime(i) <= om for i in inputs)

def _pnfs_to_xrootd(p):
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
        suffix = "/pnfs/fnal.gov" + p[len("/pnfs"):]
    return f"root://fndca1.fnal.gov:1094{suffix}"

def _hadd(outp, inputs, threads, work):
    if not inputs:
        raise RuntimeError("hadd called with no inputs")

    cmd = ["hadd", "-f", "-k"]

    if threads and threads > 1:
        d = os.path.join(work, "hadd_tmp", os.path.basename(outp).replace(".root", ""))
        os.makedirs(d, exist_ok=True)
        cmd += ["-j", str(threads), "-d", d]

    cmd += [outp] + list(inputs)
    _run(cmd)

def _merge(dest, inputs, threads, chunk, tmp):
    inputs = sorted(set(inputs))
    if not inputs:
        raise RuntimeError("no input files for " + dest)
    os.makedirs(os.path.dirname(dest), exist_ok=True)
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
                    _hadd(part, hadd_inputs[i:i+batch], threads, work)
                    partials.append(part)

                _hadd(local, partials, threads, work)
        return local, cleanup
    except Exception:
        cleanup()
        raise

def _inputs_from_outdir(outdir):
    res = []
    if not os.path.isdir(outdir):
        return res
    with os.scandir(outdir) as it:
        for e in it:
            if not e.is_dir():
                continue
            p = os.path.join(e.path, "nu_selection.root")
            if os.path.isfile(p):
                res.append(p)
    return sorted(set(res))

def _keys_and_pot(root_path, tree_path):
    t = uproot.open(f"{root_path}:{tree_path}")
    run = t["run"].array(library="np").astype(np.int64)
    sub = t["subRun"].array(library="np").astype(np.int64) & np.int64(0xFFFFFFFF)
    pot = t["pot"].array(library="np").astype(np.float64)
    key = (run << np.int64(32)) | sub
    o = np.argsort(key, kind="mergesort")
    ks = key[o]
    ps = pot[o]
    if ks.size == 0:
        return ks, 0.0
    idx = np.r_[0, np.flatnonzero(ks[1:] != ks[:-1]) + 1]
    pm = np.maximum.reduceat(ps, idx)
    ku = ks[idx]
    return ku, float(pm.sum())

def _pairs_from_keys(keys):
    r = (keys >> np.int64(32)).astype(np.int64)
    s = (keys & np.int64(0xFFFFFFFF)).astype(np.int64)
    return list(zip(r.tolist(), s.tolist()))

def _pick_col(cols, candidates):
    m = {c.lower(): c for c in cols}
    for c in candidates:
        if c.lower() in m:
            return m[c.lower()]
    raise RuntimeError("missing required column(s): " + ",".join(candidates))

def _runinfo_sums(run_db, pairs):
    if not os.path.exists(run_db):
        raise RuntimeError("run.db not found: " + run_db)
    con = sqlite3.connect(run_db)
    con.row_factory = sqlite3.Row
    cur = con.cursor()
    cur.execute("PRAGMA table_info(runinfo);")
    cols = [r[1] for r in cur.fetchall()]
    ea9c = _pick_col(cols, ["EA9CNT"])
    tortc = _pick_col(cols, ["tortgt"])
    extc = _pick_col(cols, ["EXTTrig"])
    cur.execute("PRAGMA temp_store=MEMORY;")
    cur.execute("CREATE TEMP TABLE pairs(run INTEGER, subrun INTEGER, PRIMARY KEY(run,subrun));")
    cur.executemany("INSERT OR IGNORE INTO pairs(run, subrun) VALUES (?,?);", pairs)
    row = cur.execute(f"""
        WITH subset AS (
          SELECT r.run AS run, r.subrun AS subrun, r.{ea9c} AS ea9, r.{tortc} AS tort, r.{extc} AS ext
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
    """).fetchone()
    by_run = cur.execute(f"""
        WITH subset AS (
          SELECT r.run AS run, r.subrun AS subrun, r.{extc} AS ext
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
    """).fetchall()
    con.close()
    return float(row["ea9_sum"]), float(row["tort_sum"]), float(row["ext_sum"]), int(row["matched"]), ea9c, tortc, extc, {int(r["run"]): float(r["ext_sum"]) for r in by_run}

@lru_cache(maxsize=4096)
def _prescale(run):
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

def _ext_prescaled(by_run):
    return float(sum(v * _prescale(r) for r, v in by_run.items()))

def _write_meta(root_path, nums, strs):
    import ROOT
    ROOT.gROOT.SetBatch(True)
    f = ROOT.TFile.Open(root_path, "UPDATE")
    if not f or f.IsZombie():
        raise RuntimeError("could not open ROOT file for update: " + root_path)
    for k, v in nums.items():
        ROOT.TParameter("double")(k, float(v)).Write(k, ROOT.TObject.kOverwrite)
    for k, v in strs.items():
        ROOT.TNamed(k, str(v)).Write(k, ROOT.TObject.kOverwrite)
    f.Close()

def _write_outxml(path, prod, project, merged_dir, run_db, tree, samples):
    root = ET.Element("merged_samples", attrib={
        "production_xml": prod,
        "project": project,
        "merged_dir": merged_dir,
        "run_db": run_db,
        "subrun_tree": tree,
    })
    for s in samples:
        ET.SubElement(root, "sample", attrib={k: str(v) for k, v in s.items()})
    os.makedirs(os.path.dirname(path) or ".", exist_ok=True)
    ET.ElementTree(root).write(path, encoding="utf-8", xml_declaration=True)

def main():
    base = os.getcwd()
    cfg = os.path.join(base, CONFIG)
    if not os.path.exists(cfg):
        prod = _find_prod_xml(base)
        if not prod:
            raise RuntimeError("No production XML found in this directory.")
        _, outdirs = _parse_prod_xml(prod)
        _write_cfg(cfg, prod, outdirs)
        print(f"Created {CONFIG}. Edit groups if desired, then rerun.")
        return

    prod, merged_dir, outxml, run_db, tor_scale, tree, threads, chunk, tmp, groups = _read_cfg(cfg, base)
    if not prod or not os.path.exists(prod):
        raise RuntimeError("production_xml not found")
    if not groups:
        raise RuntimeError("No groups defined in config/merge-samples.xml")

    project, stage_outdirs = _parse_prod_xml(prod)
    out_proj = os.path.join(merged_dir, project)
    os.makedirs(out_proj, exist_ok=True)

    samples = []
    for name, kind, stages in groups:
        out = os.path.join(out_proj, f"{name}.root")
        ins = []
        for st in stages:
            if st not in stage_outdirs:
                raise RuntimeError("stage not found in production XML: " + st)
            ins += _inputs_from_outdir(stage_outdirs[st])
        ins = sorted(set(ins))
        if not ins:
            print(f"Skipping {name}: no input files found.")
            continue

        print(f"Merging {name} ({kind}): {len(ins)} files")
        merged, cleanup = _merge(out, ins, threads, chunk, tmp)

        try:
            keys, pot_sum = _keys_and_pot(merged, tree)
            pairs = _pairs_from_keys(keys)

            ea9_sum, tort_raw, ext_raw, matched, ea9c, tortc, extc, ext_by_run = _runinfo_sums(run_db, pairs)
            tort_pot = float(tort_raw) * float(tor_scale)

            ext_prescaled = float(_ext_prescaled(ext_by_run))
            ext_pot_equiv = 0.0

            if kind == "ext":
                scale_to_data = (ea9_sum / ext_prescaled) if ext_prescaled > 0 else 0.0
                ext_pot_equiv = tort_pot * scale_to_data if ext_prescaled > 0 else 0.0
                normalisation = f"{ea9c}/{extc}_prescaled"
            elif kind == "data":
                scale_to_data = 1.0
                normalisation = "unity"
            else:
                scale_to_data = (tort_pot / pot_sum) if pot_sum > 0 else 0.0
                normalisation = f"{tortc}*toroid_scale/pot"

            _write_meta(
                merged,
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
                    "subruns_total": float(len(pairs)),
                    "runinfo_subruns_matched": float(matched),
                },
                {
                    "sample_name": name,
                    "sample_kind": kind,
                    "normalisation": normalisation,
                    "production_xml": prod,
                    "merge_config": cfg,
                    "run_db": run_db,
                    "ea9_column": ea9c,
                    "tortgt_column": tortc,
                    "exttrig_column": extc,
                    "subrun_tree": tree,
                    "prescale_applied": "yes" if _CONFDB is not None else "no",
                },
            )

            if _is_pnfs(out) and merged != out:
                shutil.copy2(merged, out)

            if matched != len(pairs):
                print(f"Warning: {name}: matched {matched}/{len(pairs)} subruns in runinfo")

            samples.append({
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
            })
        finally:
            cleanup()

    _write_outxml(outxml, prod, project, merged_dir, run_db, tree, samples)
    print(f"Wrote {outxml}")

if __name__ == "__main__":
    try:
        main()
    except Exception as e:
        print(str(e), file=sys.stderr)
        sys.exit(2)
