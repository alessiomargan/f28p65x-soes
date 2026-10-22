#!/usr/bin/env python3
"""Refuse bootloader/application builds with an unreviewed SOES FoE handler."""
import hashlib
import json
from pathlib import Path
import sys

root = Path(__file__).resolve().parents[1]
expected = json.loads((root / "tools/foe_stack.sha256.json").read_text())
bad = []
for relative, digest in expected.items():
    path = root / "external/SOES" / relative
    if not path.is_file() or hashlib.sha256(path.read_bytes()).hexdigest() != digest:
        bad.append(relative)
if bad:
    sys.exit("Unreviewed SOES update path: " + ", ".join(bad) +
             ". Apply the companion SOES commit and run tests/test_foe_update.py. "
             "Review changed dependency code before updating the manifest.")
print("SOES FoE completion/error handling matches the tested revision.")
