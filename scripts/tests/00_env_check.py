#!/usr/bin/env python3
from __future__ import annotations

import os
import shutil
import sys

def main() -> None:
    print("[env] python:", sys.version.replace("\n", " "))
    print("[env] cwd:", os.getcwd())

    try:
        import numpy as np
        print("[env] numpy: OK")
    except Exception as e:
        print("[env] numpy: FAIL:", e)

    try:
        import uproot
        print("[env] uproot: OK")
    except Exception as e:
        print("[env] uproot: FAIL:", e)

    hadd = shutil.which("hadd")
    print("[env] hadd:", hadd or "NOT FOUND")

    pnfs2x = shutil.which("pnfs2xrootd")
    print("[env] pnfs2xrootd:", pnfs2x or "NOT FOUND")

    try:
        import ROOT
        v = getattr(ROOT.gROOT, "GetVersion", lambda: "unknown")()
        print("[env] PyROOT: OK  ROOT version:", v)
    except Exception as e:
        print("[env] PyROOT: FAIL (some tests will skip meta checks):", e)

    print("[env] MERGE_SCRIPT:", os.environ.get("MERGE_SCRIPT", "(not set)"))
    print("[env] MERGE_CONFIG:", os.environ.get("MERGE_CONFIG", "(not set)"))

if __name__ == "__main__":
    main()
