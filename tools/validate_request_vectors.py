#!/usr/bin/env python3
"""Validate request-level fixtures without contacting firmware."""

from __future__ import annotations

import argparse
import json
from dataclasses import dataclass


REQUEST_SIZE = 4096
REQUEST_HEADER_SIZE = 48
REQUEST_DATA_CAPACITY = REQUEST_SIZE - REQUEST_HEADER_SIZE


@dataclass(frozen=True)
class Vector:
    name: str
    magic_valid: bool
    command_valid: bool
    data_size: int
    data: bytes
    sequence: int
    expected: str


def validate(vector: Vector) -> str:
    if not vector.magic_valid:
        return "invalid_magic"
    if vector.data_size < 0 or vector.data_size > REQUEST_DATA_CAPACITY:
        return "invalid_size"
    if not vector.command_valid:
        return "invalid_command"
    if vector.sequence == 0:
        return "sequence_anomaly"
    if vector.name in {"find_process_name", "find_module", "find_kernel_module"}:
        if vector.data_size == 0 or len(vector.data) < vector.data_size:
            return "invalid_size"
        if vector.data[vector.data_size - 1] != 0:
            return "invalid_size"
    return "accepted"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--json", action="store_true", help="emit JSON")
    args = parser.parse_args()
    vectors = [
        Vector("ping", True, True, 0, b"", 1, "accepted"),
        Vector("invalid_magic", False, True, 0, b"", 2, "invalid_magic"),
        Vector("unknown_command", True, False, 0, b"", 3, "invalid_command"),
        Vector("oversized", True, True, REQUEST_DATA_CAPACITY + 1, b"", 4,
               "invalid_size"),
        Vector("find_process_name", True, True, 3, b"abc", 5, "invalid_size"),
        Vector("find_process_name", True, True, 4, b"abc\0", 6, "accepted"),
        Vector("zero_sequence", True, True, 0, b"", 0, "sequence_anomaly"),
    ]
    results = []
    failed = False
    for vector in vectors:
        actual = validate(vector)
        passed = actual == vector.expected
        failed |= not passed
        results.append({"name": vector.name, "actual": actual,
                        "expected": vector.expected, "passed": passed})
    if args.json:
        print(json.dumps({"schema": "smmmem.request-vectors.v1",
                          "passed": not failed, "vectors": results}, indent=2))
    else:
        for result in results:
            print(f"{result['name']}: {result['actual']} "
                  f"({'ok' if result['passed'] else 'FAIL'})")
    return 1 if failed else 0


if __name__ == "__main__":
    raise SystemExit(main())
