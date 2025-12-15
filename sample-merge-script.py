import glob
import os
import re
import shutil
import sqlite3
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET

CONFIG = "merge-samples.xml"

def _read_text(p):
    with open(p, "r", encoding="utf-8") as f:
        return f.read()

def _write_text(p, s):
    with open(p, "w", encoding="utf-8") as f:
        f.write(s)

def _abspath(p, base):
    p = (p or "").strip()
    if not p:
        return ""
    return p if os.path.isabs(p) else os.path.abspath(os.path.join(base, p))

def _expand_entities(xml_text):
    ents = dict(re.findall(r'<!ENTITY\s+(\w+)\s+"([^"]*)"\s*>', xml_text))
    resolved = {}
    def resolve(k, stack=()):
        if k in resolved:
            return resolved[k]
        if k not in ents:
            return ""
        if k in stack:
            raise RuntimeError("circular entity: " + " -> ".join(stack + (k,)))
        v = ents[k]
        v = re.sub(r"&(\w+);", lambda m: resolve(m.group(1), stack + (k,)), v)
        resolved[k] = v
        return v
    for k in list(ents):
        resolve(k)
    xml_text = re.sub(r"&(\w+);", lambda m: resolved.get(m.group(1), m.group(0)), xml_text)
    xml_text = re.sub(r"<!DOCTYPE[\s\S]*?\]>", "", xml_text, count=1)
    return xml_text

def _parse_production_xml(path):
    txt = _expand_entities(_read_text(path))
    root = ET.fromstring(txt)
    proj = root.find("project") or root
    if proj.tag != "project":
        proj = root.find(".//project")
    if proj is None:
        raise RuntimeError("could not find <project> in production XML")
    project = (proj.attrib.get("name") or "project").strip()
    stage_outdirs = {}
    for st in proj.findall("stage"):
        name = (st.attrib.get("name") or "").strip()
        outdir = (st.findtext("outdir") or "").strip()
        if name and outdir:
            stage_outdirs[name] = outdir
    if not stage_outdirs:
        raise RuntimeError("no <stage> entries found in production XML")
    return project, stage_outdirs

def _classify(name):
    n = (name or "").lower()
    if n.startswith("ext"):
        return "ext"
    if n.startswith("dirt"):
        return "dirt"
    if "strange" in n:
        return "strangeness"
    if n.startswith("data"):
        return "data"
    return "beam"

def _find_production_xml(cwd):
    cands = [p for p in sorted(glob.glob(os.path.join(cwd, "*.xml"))) if os.path.basename(p) not in {CONFIG, "merged-samples.xml"}]
    for p in cands:
        head = _read_text(p)[:20000]
        if "<stage" in head and "<project" in head:
            return p
    return cands[0] if cands else ""

def _default_numi_db():
    for p in ("/exp/uboone/data/uboonebeam/beamdb/numi_v2.db", "/exp/uboone/data/uboonebeam/beamdb/numi_v1.db"):
        if os.path.exists(p):
            return p
    return "/exp/uboone/data/uboonebeam/beamdb/numi_v2.db"

def _write_config(cfg_path, prod_xml, project, stage_outdirs):
    kinds = {}
    for s in sorted(stage_outdirs):
        kinds.setdefault(_classify(s), []).append(s)
    order = [k for k in ("beam", "dirt", "ext", "strangeness", "data") if k in kinds] + [k for k in sorted(kinds) if k not in {"beam","dirt","ext","strangeness","data"}]
    root = ET.Element("merge_samples")
    ET.SubElement(root, "production_xml").text = os.path.basename(prod_xml)
    ET.SubElement(root, "merged_dir").text = "merged"
    ET.SubElement(root, "output_xml").text = "merged-samples.xml"
    ET.SubElement(root, "numi_db").text = _default_numi_db()
    ET.SubElement(root, "subrun_tree").text = "nuselection/SubRun"
    ET.SubElement(root, "hadd_threads").text = "8"
    ET.SubElement(root, "chunk_size").text = "250"
    ET.SubElement(root, "tmp_dir").text = os.environ.get("TMPDIR", "/tmp")
    groups = ET.SubElement(root, "groups")
    for k in order:
        g = ET.SubElement(groups, "group", attrib={"name": k, "kind": k})
        g.text = ",".join(kinds[k])
    ET.SubElement(root, "note").text = "Edit merged_dir / output_xml / groups if desired; rerun the script to merge and write normalisation metadata."
    ET.ElementTree(root).write(cfg_path, encoding="utf-8", xml_declaration=True)

def _read_config(cfg_path, base):
    root = ET.parse(cfg_path).getroot()
    prod_xml = _abspath(root.findtext("production_xml"), base)
    merged_dir = _abspath(root.findtext("merged_dir") or "merged", base)
    output_xml = _abspath(root.findtext("output_xml") or "merged-samples.xml", base)
    numi_db = _abspath(root.findtext("numi_db") or _default_numi_db(), base)
    subrun_tree = (root.findtext("subrun_tree") or "nuselection/SubRun").strip()
    hadd_threads = int((root.findtext("hadd_threads") or "8").strip())
    chunk_size = int((root.findtext("chunk_size") or "250").strip())
    tmp_dir = _abspath(root.findtext("tmp_dir") or os.environ.get("TMPDIR", "/tmp"), base)
    groups = []
    gnode = root.find("groups")
    if gnode is not None:
        for g in gnode.findall("group"):
            name = (g.attrib.get("name") or "").strip()
            kind = (g.attrib.get("kind") or name).strip()
            stages = [x.strip() for x in (g.text or "").split(",") if x.strip()]
            if name and stages:
                groups.append({"name": name, "kind": kind, "stages": stages})
    return prod_xml, merged_dir, output_xml, numi_db, subrun_tree, hadd_threads, chunk_size, tmp_dir, groups

def _is_remote(p):
    return p.startswith("/pnfs/")

def _up_to_date(outp, inputs):
    if not os.path.exists(outp):
        return False
    om = os.path.getmtime(outp)
    for i in inputs:
        if not os.path.exists(i) or os.path.getmtime(i) > om:
            return False
    return True

def _run(cmd):
    p = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    if p.returncode != 0:
        raise RuntimeError("command failed: " + " ".join(cmd) + "\n" + p.stdout)
    return p.stdout

def _hadd(outp, inputs, threads, workdir):
    cmd = ["hadd", "-f", "-k"]
    if threads > 1:
        cmd += ["-j", str(threads), "-d", os.path.join(workdir, "hadd_tmp")]
    cmd += [outp] + inputs
    _run(cmd)

def _merge(dest, inputs, threads, chunk, tmp_dir):
    inputs = [x for x in sorted(set(inputs)) if os.path.exists(x)]
    if not inputs:
        raise RuntimeError("no input files to merge for " + dest)
    os.makedirs(os.path.dirname(dest), exist_ok=True)
    if _up_to_date(dest, inputs):
        return dest
    workdir = tempfile.mkdtemp(prefix="merge_", dir=tmp_dir if os.path.isdir(tmp_dir) else None)
    try:
        local = os.path.join(workdir, "merged.root") if _is_remote(dest) else dest
        if len(inputs) == 1:
            shutil.copy2(inputs[0], local)
        elif len(inputs) <= chunk:
            _hadd(local, inputs, threads, workdir)
        else:
            parts = []
            for i in range(0, len(inputs), chunk):
                part = os.path.join(workdir, f"part_{i//chunk:05d}.root")
                _hadd(part, inputs[i:i+chunk], threads, workdir)
                parts.append(part)
            _hadd(local, parts, threads, workdir)
        if _is_remote(dest):
            shutil.copy2(local, dest)
        return dest
    finally:
        shutil.rmtree(workdir, ignore_errors=True)

def _import_root():
    import ROOT
    ROOT.gROOT.SetBatch(True)
    return ROOT

def _get_tree(f, path):
    t = f.Get(path)
    if t:
        return t
    if "/" in path:
        d, n = path.rsplit("/", 1)
        dd = f.Get(d)
        if dd:
            return dd.Get(n)
    return None

def _subrun_map(root_path, tree_path):
    ROOT = _import_root()
    f = ROOT.TFile.Open(root_path)
    if not f or f.IsZombie():
        raise RuntimeError("could not open ROOT file: " + root_path)
    t = _get_tree(f, tree_path)
    if not t:
        f.Close()
        raise RuntimeError("missing tree " + tree_path + " in " + root_path)
    m = {}
    for e in t:
        k = (int(getattr(e, "run")), int(getattr(e, "subRun")))
        v = float(getattr(e, "pot"))
        if v > m.get(k, float("-inf")):
            m[k] = v
    f.Close()
    return m

def _query_numi_db(numi_db, pairs):
    if not os.path.exists(numi_db):
        raise RuntimeError("NuMI DB not found: " + numi_db)
    con = sqlite3.connect(numi_db)
    cur = con.cursor()
    cur.execute("PRAGMA table_info(numi);")
    cols = {r[1] for r in cur.fetchall()}
    tortgt_col = "tortgt_wcut" if "tortgt_wcut" in cols else "tortgt"
    ea9_col = "EA9CNT_wcut" if "EA9CNT_wcut" in cols else "EA9CNT"
    cur.execute("CREATE TEMP TABLE rset(run INT, subrun INT, PRIMARY KEY(run,subrun));")
    cur.executemany("INSERT OR IGNORE INTO rset(run,subrun) VALUES (?,?);", list(pairs))
    sql = f"""
    WITH dedup AS (
      SELECT run, subrun, MAX({ea9_col}) AS ea9, MAX({tortgt_col}) AS tortgt
      FROM numi
      GROUP BY run, subrun
    )
    SELECT
      IFNULL(SUM(d.tortgt)*1e12, 0.0) AS tortgt_sum,
      IFNULL(SUM(d.ea9), 0.0)         AS ea9_sum,
      COUNT(*)                        AS matched
    FROM dedup d
    JOIN rset r USING(run, subrun);
    """
    cur.execute(sql)
    tortgt_sum, ea9_sum, matched = cur.fetchone()
    con.close()
    return float(tortgt_sum), float(ea9_sum), int(matched), tortgt_col, ea9_col

def _write_root_meta(root_path, numbers, strings):
    ROOT = _import_root()
    f = ROOT.TFile.Open(root_path, "UPDATE")
    if not f or f.IsZombie():
        raise RuntimeError("could not open ROOT file for update: " + root_path)
    for k, v in numbers.items():
        p = ROOT.TParameter("double")(str(k), float(v))
        p.Write(str(k), ROOT.TObject.kOverwrite)
    for k, v in strings.items():
        n = ROOT.TNamed(str(k), str(v))
        n.Write(str(k), ROOT.TObject.kOverwrite)
    f.Close()

def _find_inputs(outdir):
    res = []
    if not os.path.isdir(outdir):
        return res
    for root, _, files in os.walk(outdir):
        if "nuselection.root" in files:
            res.append(os.path.join(root, "nuselection.root"))
    return sorted(set(res))

def _write_output_xml(path, production_xml, project, merged_dir, numi_db, tree_path, samples):
    root = ET.Element("merged_samples", attrib={
        "production_xml": production_xml,
        "project": project,
        "merged_dir": merged_dir,
        "numi_db": numi_db,
        "subrun_tree": tree_path,
    })
    for s in samples:
        ET.SubElement(root, "sample", attrib={
            "name": s["name"],
            "kind": s["kind"],
            "file": s["file"],
            "subruns_total": str(s["subruns_total"]),
            "subruns_matched": str(s["subruns_matched"]),
            "pot_sum": f"{s['pot_sum']:.17g}",
            "numi_tortgt_sum": f"{s['tortgt_sum']:.17g}",
            "numi_ea9cnt_sum": f"{s['ea9_sum']:.17g}",
            "scale_to_data": f"{s['scale']:.17g}",
            "normalisation": s["normalisation"],
        })
    os.makedirs(os.path.dirname(path) or ".", exist_ok=True)
    ET.ElementTree(root).write(path, encoding="utf-8", xml_declaration=True)

def main():
    base = os.getcwd()
    cfg_path = os.path.join(base, CONFIG)
    if not os.path.exists(cfg_path):
        prod = _find_production_xml(base)
        if not prod:
            raise RuntimeError("No production XML found; place it beside this script and rerun.")
        project, stage_outdirs = _parse_production_xml(prod)
        _write_config(cfg_path, prod, project, stage_outdirs)
        print(f"Created {CONFIG} from {os.path.basename(prod)}. Edit it if desired, then rerun.")
        return

    prod_xml, merged_dir, output_xml, numi_db, tree_path, threads, chunk, tmp_dir, groups = _read_config(cfg_path, base)
    if not prod_xml or not os.path.exists(prod_xml):
        raise RuntimeError("production_xml in merge-samples.xml does not exist")
    if not groups:
        raise RuntimeError("No <group> entries in merge-samples.xml")

    project, stage_outdirs = _parse_production_xml(prod_xml)
    out_project_dir = os.path.join(merged_dir, project)
    os.makedirs(out_project_dir, exist_ok=True)

    results = []
    for g in groups:
        name, kind, stages = g["name"], g["kind"], g["stages"]
        out = os.path.join(out_project_dir, f"{name}.root")
        inputs = []
        missing = [s for s in stages if s not in stage_outdirs]
        if missing:
            raise RuntimeError("Stages not found in production XML: " + ", ".join(missing))
        for s in stages:
            inputs.extend(_find_inputs(stage_outdirs[s]))
        inputs = sorted(set(inputs))
        if not inputs:
            print(f"Skipping {name}: no input files found.")
            continue
        print(f"Merging {name} ({kind}): {len(inputs)} files")
        _merge(out, inputs, threads, chunk, tmp_dir)
        m = _subrun_map(out, tree_path)
        pot_sum = float(sum(m.values()))
        pairs = list(m.keys())
        tortgt_sum, ea9_sum, matched, tortgt_col, ea9_col = _query_numi_db(numi_db, pairs)
        if kind == "ext":
            scale = ea9_sum / pot_sum if pot_sum > 0 else 0.0
            normalisation = f"{ea9_col}/pot"
        elif kind == "data":
            scale = 1.0
            normalisation = "unity"
        else:
            scale = tortgt_sum / pot_sum if pot_sum > 0 else 0.0
            normalisation = f"{tortgt_col}/pot"
        _write_root_meta(
            out,
            {
                "pot_sum": pot_sum,
                "numi_tortgt_sum": tortgt_sum,
                "numi_ea9cnt_sum": ea9_sum,
                "scale_to_data": scale,
                "subruns_total": float(len(pairs)),
                "subruns_matched": float(matched),
            },
            {
                "sample_name": name,
                "sample_kind": kind,
                "normalisation": normalisation,
                "production_xml": prod_xml,
                "merge_config": cfg_path,
                "numi_db": numi_db,
                "numi_tortgt_column": tortgt_col,
                "numi_ea9cnt_column": ea9_col,
                "subrun_tree": tree_path,
            },
        )
        if matched != len(pairs):
            print(f"Warning: {name}: matched {matched}/{len(pairs)} subruns in the NuMI DB")
        results.append({
            "name": name,
            "kind": kind,
            "file": out,
            "subruns_total": len(pairs),
            "subruns_matched": matched,
            "pot_sum": pot_sum,
            "tortgt_sum": tortgt_sum,
            "ea9_sum": ea9_sum,
            "scale": scale,
            "normalisation": normalisation,
        })

    _write_output_xml(output_xml, prod_xml, project, merged_dir, numi_db, tree_path, results)
    print(f"Wrote {output_xml}")

if __name__ == "__main__":
    try:
        main()
    except Exception as e:
        print(str(e), file=sys.stderr)
        sys.exit(2)
