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

CONFIG = "merge-samples.xml"

DEFAULT_RUN_DB = "/exp/uboone/data/uboonebeam/beamdb/run.db"
DEFAULT_NUMI_DB = "/exp/uboone/data/uboonebeam/beamdb/numi_v3.db"

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
    if not p:
        return ""
    return p if os.path.isabs(p) else os.path.abspath(os.path.join(base, p))

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

def _write_cfg(cfg, prod, project, outdirs):
    kinds = {}
    for s in sorted(outdirs):
        kinds.setdefault(_classify(s), []).append(s)
    root = ET.Element("merge_samples")
    ET.SubElement(root, "production_xml").text = os.path.basename(prod)
    ET.SubElement(root, "reference_root").text = ""
    ET.SubElement(root, "merged_dir").text = "merged"
    ET.SubElement(root, "output_xml").text = "merged-samples.xml"
    ET.SubElement(root, "run_db").text = DEFAULT_RUN_DB
    ET.SubElement(root, "numi_db").text = DEFAULT_NUMI_DB
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
    ET.SubElement(root, "note").text = "Set reference_root to a merged on-beam data ntuple (for EA9/tortgt scaling), then rerun."
    ET.ElementTree(root).write(cfg, encoding="utf-8", xml_declaration=True)

def _read_cfg(cfg, base):
    r = ET.parse(cfg).getroot()
    prod = _abs(r.findtext("production_xml"), base)
    ref = _abs(r.findtext("reference_root"), base)
    merged = _abs(r.findtext("merged_dir") or "merged", base)
    outxml = _abs(r.findtext("output_xml") or "merged-samples.xml", base)
    run_db = _abs(r.findtext("run_db") or DEFAULT_RUN_DB, base)
    numi_db = _abs(r.findtext("numi_db") or DEFAULT_NUMI_DB, base)
    scale = float((r.findtext("toroid_scale") or "1e12").strip().replace("D", "e").replace("d", "e"))
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
    return prod, ref, merged, outxml, run_db, numi_db, scale, tree, threads, chunk, tmp, groups

def _run(cmd):
    p = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    if p.returncode != 0:
        raise RuntimeError("command failed: " + " ".join(cmd) + "\n" + p.stdout)

def _is_pnfs(p):
    return p.startswith("/pnfs/")

def _uptodate(outp, inputs):
    if not os.path.exists(outp):
        return False
    om = os.path.getmtime(outp)
    return all(os.path.exists(i) and os.path.getmtime(i) <= om for i in inputs)

def _hadd(outp, inputs, threads, work):
    cmd = ["hadd", "-f", "-k"]
    if threads > 1:
        cmd += ["-j", str(threads), "-d", os.path.join(work, "hadd_tmp")]
    cmd += [outp] + inputs
    _run(cmd)

def _merge(dest, inputs, threads, chunk, tmp):
    inputs = [x for x in sorted(set(inputs)) if os.path.exists(x)]
    if not inputs:
        raise RuntimeError("no input files for " + dest)
    os.makedirs(os.path.dirname(dest), exist_ok=True)
    if _uptodate(dest, inputs):
        return dest
    work = tempfile.mkdtemp(prefix="merge_", dir=tmp if os.path.isdir(tmp) else None)
    try:
        local = os.path.join(work, "merged.root") if _is_pnfs(dest) else dest
        if len(inputs) == 1:
            shutil.copy2(inputs[0], local)
        elif len(inputs) <= chunk:
            _hadd(local, inputs, threads, work)
        else:
            parts = []
            for i in range(0, len(inputs), chunk):
                part = os.path.join(work, f"part_{i//chunk:05d}.root")
                _hadd(part, inputs[i:i+chunk], threads, work)
                parts.append(part)
            _hadd(local, parts, threads, work)
        if _is_pnfs(dest):
            shutil.copy2(local, dest)
        return dest
    finally:
        shutil.rmtree(work, ignore_errors=True)

def _inputs_from_outdir(outdir):
    res = []
    if not os.path.isdir(outdir):
        return res
    for root, _, files in os.walk(outdir):
        if "nuselection.root" in files:
            res.append(os.path.join(root, "nuselection.root"))
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
    idx = np.r_[0, np.flatnonzero(ks[1:] != ks[:-1]) + 1]
    ku = ks[idx]
    pm = np.maximum.reduceat(ps, idx)
    return ku, float(pm.sum())

def _pairs_from_keys(keys):
    r = (keys >> np.int64(32)).astype(np.int64)
    s = (keys & np.int64(0xFFFFFFFF)).astype(np.int64)
    return list(zip(r.tolist(), s.tolist()))

def _numi_data(numi_db, pairs):
    if not os.path.exists(numi_db):
        raise RuntimeError("NuMI DB not found: " + numi_db)
    con = sqlite3.connect(numi_db)
    cur = con.cursor()
    cur.execute("PRAGMA table_info(numi);")
    cols = {r[1] for r in cur.fetchall()}
    ea9c = "EA9CNT_wcut" if "EA9CNT_wcut" in cols else "EA9CNT"
    tortc = "tortgt_wcut" if "tortgt_wcut" in cols else "tortgt"
    cur.execute("PRAGMA temp_store=MEMORY;")
    cur.execute("CREATE TEMP TABLE pairs(run INTEGER, subrun INTEGER, PRIMARY KEY(run,subrun));")
    cur.executemany("INSERT OR IGNORE INTO pairs(run, subrun) VALUES (?,?);", pairs)
    row = cur.execute(f"""
        WITH dedup AS (
          SELECT run, subrun, MAX({ea9c}) AS ea9, MAX({tortc}) AS tort
          FROM numi
          GROUP BY run, subrun
        )
        SELECT IFNULL(SUM(d.ea9), 0.0), IFNULL(SUM(d.tort), 0.0), COUNT(*)
        FROM dedup d
        JOIN pairs p ON p.run=d.run AND p.subrun=d.subrun;
    """).fetchone()
    con.close()
    return float(row[0]), float(row[1]), int(row[2]), ea9c, tortc

def _runinfo_ext(run_db, pairs):
    if not os.path.exists(run_db):
        raise RuntimeError("run.db not found: " + run_db)
    con = sqlite3.connect(run_db)
    con.row_factory = sqlite3.Row
    cur = con.cursor()
    cur.execute("PRAGMA temp_store=MEMORY;")
    cur.execute("CREATE TEMP TABLE pairs(run INTEGER, subrun INTEGER, PRIMARY KEY(run,subrun));")
    cur.executemany("INSERT OR IGNORE INTO pairs(run, subrun) VALUES (?,?);", pairs)
    raw_total = int(cur.execute("""
        SELECT IFNULL(SUM(r.EXTTrig), 0)
        FROM runinfo r
        JOIN pairs p ON p.run=r.run AND p.subrun=r.subrun;
    """).fetchone()[0])
    matched = int(cur.execute("""
        SELECT COUNT(*)
        FROM runinfo r
        JOIN pairs p ON p.run=r.run AND p.subrun=r.subrun;
    """).fetchone()[0])
    rows = cur.execute("""
        SELECT r.run AS run, IFNULL(SUM(r.EXTTrig), 0) AS ext_sum
        FROM runinfo r
        JOIN pairs p ON p.run=r.run AND p.subrun=r.subrun
        GROUP BY r.run;
    """).fetchall()
    con.close()
    by_run = {int(r["run"]): int(r["ext_sum"]) for r in rows}
    return raw_total, matched, by_run

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
    tot = 0.0
    for run, raw in by_run.items():
        tot += float(raw) * float(_prescale(run))
    return tot

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

def _write_outxml(path, prod, project, merged_dir, ref, numi_db, run_db, tree, ea9c, tortc, data_ea9, data_tort_raw, data_tort_pot, samples):
    root = ET.Element("merged_samples", attrib={
        "production_xml": prod,
        "project": project,
        "merged_dir": merged_dir,
        "reference_root": ref,
        "numi_db": numi_db,
        "run_db": run_db,
        "subrun_tree": tree,
        "ea9_column": ea9c,
        "tortgt_column": tortc,
        "data_ea9_wcut": f"{data_ea9:.17g}",
        "data_tortgt_raw": f"{data_tort_raw:.17g}",
        "data_tortgt_pot": f"{data_tort_pot:.17g}",
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
        project, outdirs = _parse_prod_xml(prod)
        _write_cfg(cfg, prod, project, outdirs)
        print(f"Created {CONFIG}. Set reference_root, then rerun.")
        return

    prod, ref, merged_dir, outxml, run_db, numi_db, tor_scale, tree, threads, chunk, tmp, groups = _read_cfg(cfg, base)
    if not prod or not os.path.exists(prod):
        raise RuntimeError("production_xml not found")
    if not ref or not os.path.exists(ref):
        raise RuntimeError("reference_root not found (set it in merge-samples.xml)")
    if not groups:
        raise RuntimeError("No groups defined in merge-samples.xml")

    project, stage_outdirs = _parse_prod_xml(prod)
    out_proj = os.path.join(merged_dir, project)
    os.makedirs(out_proj, exist_ok=True)

    ref_keys, _ = _keys_and_pot(ref, tree)
    ref_pairs = _pairs_from_keys(ref_keys)
    data_ea9, data_tort_raw, data_matched, ea9c, tortc = _numi_data(numi_db, ref_pairs)
    data_tort_pot = float(data_tort_raw) * float(tor_scale)

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
        _merge(out, ins, threads, chunk, tmp)

        keys, pot_sum = _keys_and_pot(out, tree)
        pairs = _pairs_from_keys(keys)

        scale_to_data = 1.0
        normalisation = "unity"
        ext_raw = 0
        ext_prescaled = 0.0
        ext_pot_equiv = 0.0
        numi_matched = data_matched
        run_matched = 0

        if kind == "ext":
            ext_raw, run_matched, by_run = _runinfo_ext(run_db, pairs)
            ext_prescaled = float(_ext_prescaled(by_run))
            scale_to_data = (data_ea9 / ext_prescaled) if ext_prescaled > 0 else 0.0
            ext_pot_equiv = data_tort_pot * scale_to_data if ext_prescaled > 0 else 0.0
            normalisation = "EA9CNT_wcut/EXTTrig_prescaled"
        elif kind == "data":
            scale_to_data = 1.0
            normalisation = "unity"
        else:
            scale_to_data = (data_tort_pot / pot_sum) if pot_sum > 0 else 0.0
            normalisation = "tortgt_wcut/pot"

        _write_meta(
            out,
            {
                "pot_sum": pot_sum,
                "scale_to_data": scale_to_data,
                "data_ea9_wcut": data_ea9,
                "data_tortgt_raw": data_tort_raw,
                "data_tortgt_pot": data_tort_pot,
                "toroid_scale": tor_scale,
                "subruns_total": float(len(pairs)),
                "numi_matched": float(numi_matched),
                "run_matched": float(run_matched),
                "exttrig_raw": float(ext_raw),
                "exttrig_prescaled": float(ext_prescaled),
                "ext_pot_equiv": float(ext_pot_equiv),
            },
            {
                "sample_name": name,
                "sample_kind": kind,
                "normalisation": normalisation,
                "production_xml": prod,
                "merge_config": cfg,
                "reference_root": ref,
                "numi_db": numi_db,
                "run_db": run_db,
                "ea9_column": ea9c,
                "tortgt_column": tortc,
                "subrun_tree": tree,
                "prescale_applied": "yes" if _CONFDB is not None else "no",
            },
        )

        if kind == "ext" and ext_prescaled <= 0:
            print(f"Warning: {name}: EXTTrig prescaled sum is zero; normalisation will be zero.")
        if kind == "ext" and run_matched != len(pairs):
            print(f"Warning: {name}: matched {run_matched}/{len(pairs)} subruns in run.db")

        samples.append({
            "name": name,
            "kind": kind,
            "file": out,
            "subruns_total": len(pairs),
            "pot_sum": f"{pot_sum:.17g}",
            "scale_to_data": f"{scale_to_data:.17g}",
            "normalisation": normalisation,
            "exttrig_raw": ext_raw,
            "exttrig_prescaled": f"{ext_prescaled:.17g}",
            "ext_pot_equiv": f"{ext_pot_equiv:.17g}",
        })

    _write_outxml(outxml, prod, project, merged_dir, ref, numi_db, run_db, tree, ea9c, tortc, data_ea9, data_tort_raw, data_tort_pot, samples)
    print(f"Wrote {outxml}")

if __name__ == "__main__":
    try:
        main()
    except Exception as e:
        print(str(e), file=sys.stderr)
        sys.exit(2)
