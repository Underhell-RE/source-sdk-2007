#!/usr/bin/env python3
#=============================================================================
# diaphora_filter.py — filter a Diaphora SQLite export into trust tiers.
#
# Diaphora matches a modded DLL (e.g. Underhell Cliento.dll) against the
# vanilla DLL it forked from (e.g. original/Client.dll). The match is NOT
# ground truth in the regions the mod rewrote: there, custom functions get
# spurious "ratio 1.0" matches to nearby unrelated vanilla functions.
#
# The reliable signal is the ADDRESS DELTA (primary - secondary):
#   * genuine matches (same function, shifted by the mod's inserted code)
#     cluster at the image shift — large positive (~0x13000 for Cliento.dll).
#   * false matches (different function that happens to be near) have small,
#     zero, or negative deltas.
#
# This script splits the `results` table into three tiers:
#   reliable   — "best" matches with delta >= --min-delta  (trusted: vanilla).
#   verify     — everything else (multimatch/partial, or best with small
#                delta) — read the code before trusting the name.
#   mod_delta  — the `unmatched` table (functions with NO vanilla counterpart,
#                i.e. the mod's own additions to reverse).
#
# Usage:
#   python3 diaphora_filter.py cliento.diaphora [--min-delta 0x10000] [--out DIR]
#
# Outputs (written next to the .diaphora, or into --out):
#   <stem>_reliable.csv, <stem>_verify.csv, <stem>_mod_delta.csv
#   plus a delta histogram printed to stdout.
#=============================================================================

import argparse
import csv
import os
import sqlite3
import sys
from collections import Counter


def parse_addr(s):
    """Hex string ('10001000' or '0x10001000') -> int. Returns None on garbage."""
    s = s.strip()
    if not s:
        return None
    return int(s, 16)


def iter_rows(cur, table, cols):
    cur.execute("SELECT %s FROM %s" % (", ".join(cols), table))
    for row in cur.fetchall():
        yield dict(zip(cols, row))


def main():
    ap = argparse.ArgumentParser(description="Split a Diaphora export into trust tiers.")
    ap.add_argument("db", help="path to the .diaphora SQLite file")
    ap.add_argument("--min-delta", default="0x10000",
                    help="address delta (hex) above which a 'best' match is trusted "
                         "(default 0x10000)")
    ap.add_argument("--out", default=None, help="output directory (default: next to db)")
    args = ap.parse_args()

    min_delta = int(args.min_delta, 16)

    if not os.path.exists(args.db):
        sys.exit("no such file: %s" % args.db)

    con = sqlite3.connect(args.db)
    cur = con.cursor()

    tables = [r[0] for r in cur.execute(
        "SELECT name FROM sqlite_master WHERE type='table'")]
    if "results" not in tables or "unmatched" not in tables:
        sys.exit("not a Diaphora sqlite (missing 'results'/'unmatched' tables): %s"
                 % tables)

    out_dir = args.out or os.path.dirname(os.path.abspath(args.db))
    os.makedirs(out_dir, exist_ok=True)
    stem = os.path.splitext(os.path.basename(args.db))[0]

    reliable = []
    verify = []
    delta_hist = Counter()

    for r in iter_rows(cur, "results", ["type", "address", "name", "address2", "name2", "ratio"]):
        a = parse_addr(r["address"])
        b = parse_addr(r["address2"])
        if a is None or b is None:
            continue
        delta = a - b
        delta_hist[delta] += 1

        row = [r["address"], r["name"], r["address2"], r["name2"], r["ratio"], "%#x" % (delta & 0xffffffff)]

        # Trust only unambiguous "best" matches that are clearly the same
        # function shifted by the mod's code insertions (large positive delta).
        if r["type"] == "best" and delta >= min_delta:
            reliable.append(row)
        else:
            verify.append(row)

    mod_delta = [[r["name"], r["address"]] for r in
                 iter_rows(cur, "unmatched", ["name", "address"])]

    # Write CSVs.
    def write(path, header, rows):
        with open(os.path.join(out_dir, path), "w", newline="") as f:
            w = csv.writer(f)
            w.writerow(header)
            w.writerows(rows)
        return os.path.join(out_dir, path)

    p_reliable = write("%s_reliable.csv" % stem,
                       ["address", "name", "address2", "name2", "ratio", "delta"],
                       reliable)
    p_verify = write("%s_verify.csv" % stem,
                     ["address", "name", "address2", "name2", "ratio", "delta"],
                     verify)
    p_mod = write("%s_mod_delta.csv" % stem, ["name", "address"], mod_delta)

    # Summary.
    print("Diaphora filter: %s" % args.db)
    print("  reliable   (best, delta >= %#x): %d" % (min_delta, len(reliable)))
    print("  verify     (multimatch/partial/small delta): %d" % len(verify))
    print("  mod_delta  (unmatched = mod's own code): %d" % len(mod_delta))
    print("\nDelta histogram (top 12):")
    for d, c in delta_hist.most_common(12):
        print("    %10s -> %d" % ("%#x" % (d & 0xffffffff), c))
    print("\nWrote:")
    print("  " + p_reliable)
    print("  " + p_verify)
    print("  " + p_mod)
    print("\nRule of thumb: trust `reliable` names; for `verify`, read the code\n"
          "(false positives concentrate in the regions the mod rewrote — HUD,\n"
          "inventory, weapons). `mod_delta` is the list of functions to reverse.")


if __name__ == "__main__":
    main()
