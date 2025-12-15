import argparse
import glob
import hashlib
import os
import re
import shutil
import sqlite3
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET

def expand_entities(xml_text: str) -> str:
    ents = dict(re.findall(r'<!ENTITY\s+(\w+)\s+"([^"]*)"\s*>', xml_text))
    resolved = {}
    def resolve(k, stack=()):
        if k in resolved:
            return resolved[k]
        if k not in ents:
            return ""
        if k in stack:
            raise RuntimeError(f"circular entity: {' -> '.join(stack + (k,))}")
        v = ents[k]
        v = re.sub(r"&(\w+);", lambda m: resolve(m.group(1), stack + (k,)), v)
        resolved[k] = v
        return v
    for k in list(ents.keys()):
        resolve(k)
    xml_text = re.sub(r"&(\w+);", lambda m: resolved.get(m.group(1), m.group(0)), xml_text)
    xml_text = re.sub(r"<!DOCTYPE[\s\S]*?\]>", "", xml_text, count=1)
    return xml_text

def parse_production_xml(path: str):
    txt = open(path, "r", encoding="utf-8").read()
    txt = expand_entities(txt)
    root = ET.fromstring(txt)
    proj = root.find("project")
    if proj is None:
        raise RuntimeError("could not find <project> in XML")
    proj_name = proj.attrib.get("name", "project")
    stages = []
    for st in proj.findall("stage"):
        name = st.attrib.get("name", "").strip()
        outdir = (st.findtext("outdir") or "").strip()
        inputdef = (st.findtext("inputdef") or "").strip()
        if name and outdir:
            stages.append({"name": name, "outdir": outdir, "inputdef": inputdef})
    return proj_name, stages

def classify_stage(stage_name: str, inputdef: str) -> str:
    n = (stage_name or "").lower()
    if n.startswith("ext"):
        return "ext"
    if n.startswith("dirt"):
        return "dirt"
    if "strange" in n:
        return "strangeness"
    if n.startswith("data") or " data" in (" " + n + " "):
        return "data"
    return "beam"

def find_outputs(outdir: str):
    res = []
    if not os.path.isdir(outdir):
        return res
    for root, _, files in os.walk(outdir):
        if "nuselection.root" in files:
            res.append(os.path.join(root, "nuselection.root"))
    return sorted(set(res))

def run_cmd(cmd):
    p = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    if p.returncode != 0:
        raise RuntimeError(f"command failed ({p.returncode}): {' '.join(cmd)}\n{p.stdout}")
    return p.stdout

def is_pnfs(path: str) -> bool:
    return path.startswith("/pnfs/")

def up_to_date(out: str, inputs):
    if not os.path.exists(out):
        return False
    om = os.path.getmtime(out)
    for x in inputs:
        try:
            if os.path.getmtime(x) > om:
                return False
        except FileNotFoundError:
            return False
    return True

def hadd_merge(dest: str, inputs, hadd_j: int, chunk: int, tmp_root: str, force: bool):
    inputs = [x for x in sorted(set(inputs)) if os.path.exists(x)]
    if not inputs:
        raise RuntimeError(f"no input files for {dest}")
    os.makedirs(os.path.dirname(dest), exist_ok=True)
    if (not force) and up_to_date(dest, inputs):
        return dest
    work = tempfile.mkdtemp(prefix="merge_", dir=tmp_root)
    try:
        def do_hadd(outp: str, ins):
            cmd = ["hadd", "-f", "-k"]
            if hadd_j and hadd_j > 1:
                cmd += ["-j", str(hadd_j), "-d", os.path.join(work, "hadd_tmp")]
            cmd += [outp] + ins
            run_cmd(cmd)
        if len(inputs) == 1:
            local_out = os.path.join(work, "single.root") if is_pnfs(dest) else dest
            shutil.copy2(inputs[0], local_out)
        else:
            parts = []
            chunks = [inputs[i:i+chunk] for i in range(0, len(inputs), chunk)]
            if len(chunks) == 1:
                local_out = os.path.join(work, "merged.root") if is_pnfs(dest) else dest
                do_hadd(local_out, chunks[0])
            else:
                for i, ch in enumerate(chunks):
                    pth = os.path.join(work, f"part_{i:05d}.root")
                    do_hadd(pth, ch)
                    parts.append(pth)
                local_out = os.path.join(work, "merged.root") if is_pnfs(dest) else dest
                do_hadd(local_out, parts)
        if is_pnfs(dest):
            shutil.copy2(local_out, dest)
        return dest
    finally:
        shutil.rmtree(work, ignore_errors=True)

def import_root():
    import ROOT
    ROOT.gROOT.SetBatch(True)
    return ROOT

def get_tree(f, path: str):
    t = f.Get(path)
    if t:
        return t
    if "/" in path:
        d, n = path.rsplit("/", 1)
        dd = f.Get(d)
        if dd:
            return dd.Get(n)
    return None

def subrun_sum_pot(root_path: str, tree_path: str):
    ROOT = import_root()
    f = ROOT.TFile.Open(root_path)
    if not f or f.IsZombie():
        raise RuntimeError(f"failed to open ROOT file: {root_path}")
    t = get_tree(f, tree_path)
    if not t:
        f.Close()
        raise RuntimeError(f"missing tree {tree_path} in {root_path}")
    s = 0.0
    for e in t:
        s += float(getattr(e, "pot"))
    f.Close()
    return s

def subrun_pairs(root_path: str, tree_path: str):
    ROOT = import_root()
    f = ROOT.TFile.Open(root_path)
    if not f or f.IsZombie():
        raise RuntimeError(f"failed to open ROOT file: {root_path}")
    t = get_tree(f, tree_path)
    if not t:
        f.Close()
        raise RuntimeError(f"missing tree {tree_path} in {root_path}")
    pairs = set()
    for e in t:
        pairs.add((int(getattr(e, "run")), int(getattr(e, "subRun"))))
    f.Close()
    return pairs

def query_numi_db(numi_db: str, pairs):
    if not os.path.exists(numi_db):
        raise RuntimeError(f"numi DB not found: {numi_db}")
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
      SELECT run, subrun,
             MAX({ea9_col}) AS ea9,
             MAX({tortgt_col}) AS tortgt
      FROM numi
      GROUP BY run, subrun
    )
    SELECT
      IFNULL(SUM(d.tortgt)*1e12, 0.0) AS tortgt_sum,
      IFNULL(SUM(d.ea9), 0.0) AS ea9_sum
    FROM dedup d
    JOIN rset r USING(run, subrun);
    """
    cur.execute(sql)
    tortgt_sum, ea9_sum = cur.fetchone()
    con.close()
    return float(tortgt_sum), float(ea9_sum), tortgt_col, ea9_col

def write_root_meta(root_path: str, numbers: dict, strings: dict):
    ROOT = import_root()
    f = ROOT.TFile.Open(root_path, "UPDATE")
    if not f or f.IsZombie():
        raise RuntimeError(f"failed to open ROOT file for update: {root_path}")
    for k, v in numbers.items():
        p = ROOT.TParameter("double")(str(k), float(v))
        p.Write(str(k), ROOT.TObject.kOverwrite)
    for k, v in strings.items():
        n = ROOT.TNamed(str(k), str(v))
        n.Write(str(k), ROOT.TObject.kOverwrite)
    f.Close()

def normalize_path(p: str) -> str:
    return os.path.abspath(os.path.expanduser(p))

def default_numi_db():
    cands = [
        "/exp/uboone/data/uboonebeam/beamdb/numi_v2.db",
        "/exp/uboone/data/uboonebeam/beamdb/numi_v1.db",
    ]
    for c in cands:
        if os.path.exists(c):
            return c
    return cands[0]

def collect_pairs_from_data_arg(data_arg: str, tree_path: str):
    if not data_arg:
        return set()
    p = normalize_path(data_arg)
    files = []
    if os.path.isfile(p) and p.endswith(".root"):
        files = [p]
    elif os.path.isdir(p):
        files = []
        for root, _, fn in os.walk(p):
            if "nuselection.root" in fn:
                files.append(os.path.join(root, "nuselection.root"))
        if not files:
            files = glob.glob(os.path.join(p, "**", "*.root"), recursive=True)
    else:
        files = glob.glob(data_arg, recursive=True)
    pairs = set()
    for f in sorted(set(files)):
        try:
            pairs |= subrun_pairs(f, tree_path)
        except Exception:
            continue
    return pairs

def build_output_xml(path: str, source_xml: str, project: str, numi_db: str, tortgt_col: str, ea9_col: str, data_tortgt: float, data_ea9: float, stages, groups):
    root = ET.Element("merged_project", attrib={"source": source_xml, "name": project})
    data = ET.SubElement(root, "data_exposure", attrib={
        "numi_db": numi_db,
        "tortgt_column": tortgt_col,
        "ea9cnt_column": ea9_col,
        "tortgt_sum": f"{data_tortgt:.17g}",
        "ea9cnt_sum": f"{data_ea9:.17g}",
    })
    ET.SubElement(data, "note").text = "tortgt is reported in POT (db value * 1e12); ea9cnt is reported as counts"
    st_el = ET.SubElement(root, "stages")
    for s in stages:
        ET.SubElement(st_el, "stage", attrib={
            "name": s["name"],
            "kind": s["kind"],
            "inputdef": s.get("inputdef", ""),
            "outdir": s.get("outdir", ""),
            "merged": s.get("merged", ""),
            "pot_or_ntrig_sum": f"{s.get('sum_exposure', 0.0):.17g}",
            "scale_to_data": f"{s.get('scale', 0.0):.17g}" if s.get("scale") is not None else "",
            "group": s.get("group", ""),
        })
    gp_el = ET.SubElement(root, "groups")
    for g in groups:
        ET.SubElement(gp_el, "group", attrib={
            "name": g["name"],
            "kind": g["kind"],
            "merged": g.get("merged", ""),
            "pot_or_ntrig_sum": f"{g.get('sum_exposure', 0.0):.17g}",
            "scale_to_data": f"{g.get('scale', 0.0):.17g}",
            "components": ",".join(g.get("components", [])),
        })
    tree = ET.ElementTree(root)
    os.makedirs(os.path.dirname(path), exist_ok=True)
    tree.write(path, encoding="utf-8", xml_declaration=True)

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("production_xml")
    ap.add_argument("merged_dir")
    ap.add_argument("output_xml")
    ap.add_argument("--numi-db", default=default_numi_db())
    ap.add_argument("--data", default="")
    ap.add_argument("--subrun-tree", default="nuselection/SubRun")
    ap.add_argument("--hadd-j", type=int, default=max(1, (os.cpu_count() or 8) // 2))
    ap.add_argument("--chunk", type=int, default=250)
    ap.add_argument("--tmp", default=os.environ.get("TMPDIR", "/tmp"))
    ap.add_argument("--force", action="store_true")
    args = ap.parse_args()

    prod_xml = normalize_path(args.production_xml)
    merged_dir = normalize_path(args.merged_dir)
    out_xml = normalize_path(args.output_xml)
    numi_db = normalize_path(args.numi_db)
    tmp_root = normalize_path(args.tmp)

    project, stages = parse_production_xml(prod_xml)
    for s in stages:
        s["kind"] = classify_stage(s["name"], s.get("inputdef", ""))
        s["files"] = find_outputs(s["outdir"])

    stages = [s for s in stages if s["files"]]
    if not stages:
        raise RuntimeError("no stages with output ROOT files were found")

    proj_dir = os.path.join(merged_dir, project)
    os.makedirs(proj_dir, exist_ok=True)

    for s in stages:
        s["merged"] = os.path.join(proj_dir, f"{s['name']}.root")
        hadd_merge(s["merged"], s["files"], args.hadd_j, args.chunk, tmp_root, args.force)
        s["sum_exposure"] = subrun_sum_pot(s["merged"], args.subrun_tree)

    data_stages = [s for s in stages if s["kind"] == "data"]
    data_pairs = set()
    for s in data_stages:
        data_pairs |= subrun_pairs(s["merged"], args.subrun_tree)
    if not data_pairs:
        data_pairs = collect_pairs_from_data_arg(args.data, args.subrun_tree)
    if not data_pairs:
        raise RuntimeError("no data run/subRun pairs found; provide --data pointing to a merged data ROOT file (or a directory/glob of data outputs)")

    data_tortgt, data_ea9, tortgt_col, ea9_col = query_numi_db(numi_db, data_pairs)

    groups = {}
    for s in stages:
        groups.setdefault(s["kind"], []).append(s)

    group_outputs = []
    for kind, ss in sorted(groups.items(), key=lambda kv: kv[0]):
        comp = sorted([x["name"] for x in ss])
        sum_exposure = float(sum(x["sum_exposure"] for x in ss))
        if kind == "ext":
            scale = (data_ea9 / sum_exposure) if sum_exposure > 0 else 0.0
            scale_kind = "ea9cnt_over_ntrig"
        elif kind == "data":
            scale = 1.0
            scale_kind = "unity"
        else:
            scale = (data_tortgt / sum_exposure) if sum_exposure > 0 else 0.0
            scale_kind = "tortgt_over_potmc"
        if len(ss) > 1:
            gfile = os.path.join(proj_dir, f"{kind}.root")
            hadd_merge(gfile, [x["merged"] for x in ss], args.hadd_j, args.chunk, tmp_root, args.force)
        else:
            gfile = ss[0]["merged"]
        write_root_meta(
            gfile,
            {
                "exposure_sum": sum_exposure,
                "data_tortgt_sum": data_tortgt,
                "data_ea9cnt_sum": data_ea9,
                "scale_to_data": scale,
            },
            {
                "sample_kind": kind,
                "sample_group": kind,
                "scale_kind": scale_kind,
                "numi_db": numi_db,
                "numi_tortgt_column": tortgt_col,
                "numi_ea9cnt_column": ea9_col,
                "project": project,
            },
        )
        for s in ss:
            s["group"] = kind
            s["scale"] = scale
            write_root_meta(
                s["merged"],
                {
                    "exposure_sum": float(s["sum_exposure"]),
                    "group_exposure_sum": sum_exposure,
                    "data_tortgt_sum": data_tortgt,
                    "data_ea9cnt_sum": data_ea9,
                    "scale_to_data": scale,
                },
                {
                    "sample_kind": kind,
                    "sample_group": kind,
                    "sample_stage": s["name"],
                    "scale_kind": scale_kind,
                    "numi_db": numi_db,
                    "numi_tortgt_column": tortgt_col,
                    "numi_ea9cnt_column": ea9_col,
                    "project": project,
                },
            )
        group_outputs.append({"name": kind, "kind": kind, "merged": gfile, "sum_exposure": sum_exposure, "scale": scale, "components": comp})

    build_output_xml(out_xml, prod_xml, project, numi_db, tortgt_col, ea9_col, data_tortgt, data_ea9, stages, group_outputs)

if __name__ == "__main__":
    try:
        main()
    except Exception as e:
        print(str(e), file=sys.stderr)
        sys.exit(2)
