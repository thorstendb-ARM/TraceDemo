#!/usr/bin/env python3
"""Summarize a CMSIS trace CSV and optionally resolve PCs with addr2line."""

import argparse
import csv
import os
import shutil
import subprocess
from collections import Counter, defaultdict


def parse_int(value):
    try:
        return int(value, 0)
    except (TypeError, ValueError):
        return None


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--csv", required=True)
    parser.add_argument("--elf")
    parser.add_argument("--trace-clock", type=float)
    args = parser.parse_args()

    events = []
    with open(args.csv, newline="") as stream:
        for row in csv.DictReader(stream):
            events.append(row)

    print(f"events: {len(events)}")
    print("types:", ", ".join(f"{k}={v}" for k, v in Counter(r.get("type", "") for r in events).most_common()))

    pc_counts = Counter(r.get("pc", "") for r in events if r.get("pc"))
    print("pcs:")
    for pc, count in pc_counts.most_common():
        print(f"  {pc}: {count}")

    groups = defaultdict(Counter)
    for row in events:
        if row.get("type") == "dwt":
            groups[row.get("pc", "")][row.get("value", "")] += 1
    if groups:
        print("dwt values by pc:")
        for pc, values in groups.items():
            detail = ", ".join(f"{value}={count}" for value, count in values.most_common())
            print(f"  {pc}: {detail}")

    cycles = [parse_int(r.get("cycles")) for r in events]
    cycles = [c for c in cycles if c is not None]
    if cycles:
        print(f"cycle range: {min(cycles)}..{max(cycles)}")
        if args.trace_clock:
            print(f"time range: {min(cycles) / args.trace_clock:.9f}..{max(cycles) / args.trace_clock:.9f} s")

    if args.elf:
        addr2line = shutil.which("arm-none-eabi-addr2line")
        if addr2line and os.path.exists(args.elf):
            print("source locations:")
            for pc in pc_counts:
                if parse_int(pc) is None:
                    continue
                result = subprocess.run(
                    [addr2line, "-e", args.elf, "-f", "-C", "-i", pc],
                    capture_output=True, text=True, check=False,
                )
                lines = [line for line in result.stdout.splitlines() if line]
                print(f"  {pc}: {' | '.join(lines)}")
        else:
            print("source locations: unavailable (addr2line or ELF not found)")


if __name__ == "__main__":
    main()
