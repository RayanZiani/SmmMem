#!/usr/bin/env python3
"""Check protocol constants shared by the source trees and documentation.

This is an offline consistency check. It reads repository files only; it does
not contact WMI, firmware, or a target machine.
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def define(text: str, name: str) -> str:
    match = re.search(rf"^\s*#define\s+{re.escape(name)}\s+([^\r\n/]+)", text, re.MULTILINE)
    if match is None:
        raise ValueError(f"missing #define {name}")
    return match.group(1).strip()


def integer(value: str) -> int:
    value = value.strip().rstrip("uUlL")
    return int(value, 0)


def check_define(path: Path, name: str, expected: int, failures: list[str]) -> None:
    try:
        actual = integer(define(read(path), name))
    except (OSError, ValueError) as error:
        failures.append(f"{path.relative_to(ROOT)}: {error}")
        return
    if actual != expected:
        failures.append(
            f"{path.relative_to(ROOT)}: {name}={actual}, expected {expected}"
        )


def check_text(path: Path, expected: str, failures: list[str]) -> None:
    try:
        text = read(path)
    except OSError as error:
        failures.append(f"{path.relative_to(ROOT)}: {error}")
        return
    if expected not in text:
        failures.append(f"{path.relative_to(ROOT)}: missing {expected!r}")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--quiet", action="store_true", help="print only failures")
    args = parser.parse_args()

    failures: list[str] = []
    src = ROOT / "src"
    debug = ROOT / "src_dbg01"
    mapper = ROOT / "mapper"
    tools = ROOT / "tools"

    # The release and debug memory transports must share the same wire layout.
    for tree in (src, debug):
        common = tree / "Common.h"
        for name, expected in (
            ("MAILBOX_SIZE", 0x2000),
            ("REQUEST_SIZE", 4096),
            ("RESPONSE_OFFSET", 0x1000),
            ("RESPONSE_SIZE", 512),
            ("RESPONSE_DATA_SIZE", 352),
            ("SW_SMI_VALUE", 0xD6),
        ):
            check_define(common, name, expected, failures)

        client = tree / "Client.c"
        for name, expected in (
            ("REQUEST_SIZE", 4096),
            ("RESPONSE_SIZE", 512),
            ("RESPONSE_DATA_SIZE", 352),
        ):
            check_define(client, name, expected, failures)

    check_define(tools / "WmiPingBench.c", "REQUEST_SIZE", 4096, failures)
    check_define(tools / "WmiPingBench.c", "RESPONSE_SIZE", 512, failures)

    # Mapper request/response values are intentionally separate from src/.
    for path in (mapper / "DxeBridge.c", mapper / "SmmHost.c"):
        for name, expected in (
            ("WMI_REQUEST_SIZE", 4096),
            ("WMI_RESPONSE_OFFSET", 0xD00),
            ("WMI_RESPONSE_SIZE", 512),
            ("SW_SMI_VALUE", 0xD5),
        ):
            check_define(path, name, expected, failures)

    check_define(mapper / "SmmClient.c", "WMI_REQUEST_SIZE", 4096, failures)
    check_define(mapper / "SmmClient.c", "WMI_RESPONSE_SIZE", 512, failures)
    check_define(mapper / "SmmClient.c", "WMI_REQUEST_DATA_CAPACITY", 4000, failures)

    # Keep the project-owned identifiers and instance names attributable.
    for path in (src / "Client.c", debug / "Client.c"):
        check_text(path, "0xa0c9f8de", failures)
        for instance in ("Mem_0", "SMMM_0", "0_0"):
            check_text(path, instance, failures)
    check_text(mapper / "SmmClient.c", "0x9b6f1a20", failures)
    for instance in ("SmmMapper_0", "SMMP_0", "0_0"):
        check_text(mapper / "SmmClient.c", instance, failures)

    # Check the documented capacities, including the release/debug fix.
    transport_doc = ROOT / "docs" / "WMI_TRANSPORT.md"
    for phrase in (
        "`src/` memory API",
        "| 4096 | bounded by request fields | 512 | 352 |",
        "`mapper/` control API",
        "| 4096 | 4000 | 512 | 464 |",
    ):
        check_text(transport_doc, phrase, failures)

    if failures:
        for failure in failures:
            print(f"FAIL: {failure}", file=sys.stderr)
        return 1

    if not args.quiet:
        print("protocol consistency: ok")
        print("checked: src, src_dbg01, mapper, WmiPingBench, WMI transport docs")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
