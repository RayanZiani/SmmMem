#!/usr/bin/env python3
"""Validate a WmiPingBench CSV and emit a stable JSON summary."""

from __future__ import annotations

import argparse
import csv
import json
import statistics
import sys
from pathlib import Path


HEADER = (
    "protocol",
    "command",
    "request_size",
    "response_capacity",
    "iteration",
    "instance",
    "elapsed_us",
    "wmi_status",
    "response_status",
)


def parse_status(value: str) -> int:
    return int(value, 0)


def read_rows(path: Path) -> list[dict[str, object]]:
    with path.open("r", encoding="utf-8-sig", newline="") as stream:
        reader = csv.DictReader(stream)
        if tuple(reader.fieldnames or ()) != HEADER:
            raise ValueError(
                f"{path}: expected header {HEADER!r}, got {reader.fieldnames!r}"
            )
        rows: list[dict[str, object]] = []
        for line_number, row in enumerate(reader, start=2):
            try:
                parsed = {
                    "protocol": row["protocol"],
                    "command": row["command"],
                    "request_size": int(row["request_size"]),
                    "response_capacity": int(row["response_capacity"]),
                    "iteration": int(row["iteration"]),
                    "instance": row["instance"],
                    "elapsed_us": float(row["elapsed_us"]),
                    "wmi_status": parse_status(row["wmi_status"]),
                    "response_status": parse_status(row["response_status"]),
                }
            except (KeyError, TypeError, ValueError) as error:
                raise ValueError(f"{path}:{line_number}: invalid row: {error}") from error
            rows.append(parsed)
    if not rows:
        raise ValueError(f"{path}: no benchmark rows")
    return rows


def summarize(rows: list[dict[str, object]]) -> dict[str, object]:
    durations = [float(row["elapsed_us"]) for row in rows]
    failures = [
        row
        for row in rows
        if int(row["wmi_status"]) != 0 or int(row["response_status"]) != 0
    ]
    return {
        "schema": "smmmem.wmi-ping.v1",
        "protocol": rows[0]["protocol"],
        "command": rows[0]["command"],
        "request_size": rows[0]["request_size"],
        "response_capacity": rows[0]["response_capacity"],
        "samples": len(rows),
        "failures": len(failures),
        "instances": sorted({str(row["instance"]) for row in rows}),
        "latency_us": {
            "min": min(durations),
            "mean": statistics.fmean(durations),
            "median": statistics.median(durations),
            "max": max(durations),
        },
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", type=Path, help="WmiPingBench CSV")
    parser.add_argument("-o", "--output", type=Path, help="JSON output path")
    parser.add_argument("--fail-on-error", action="store_true")
    args = parser.parse_args()
    try:
        result = summarize(read_rows(args.input))
    except (OSError, ValueError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 2
    encoded = json.dumps(result, indent=2, sort_keys=True) + "\n"
    if args.output:
        args.output.write_text(encoded, encoding="utf-8", newline="\n")
    else:
        print(encoded, end="")
    return 1 if args.fail_on_error and result["failures"] else 0


if __name__ == "__main__":
    raise SystemExit(main())
