#!/usr/bin/env python3
"""Compare two read-only WMI inventory snapshots.

The input format is the CSV emitted by WmiInventory.exe:
    namespace,class_name

Provider snapshots use:
    name,clsid,hosting_model

The comparison is deliberately data-only. It does not query WMI and does not
make any changes to the host.
"""

from __future__ import annotations

import argparse
import csv
import sys
from collections import Counter
from pathlib import Path


SUPPORTED_HEADERS = {
    ("namespace", "class_name"),
    ("name", "clsid", "hosting_model"),
}


def read_snapshot(path: Path) -> tuple[tuple[str, ...], Counter[tuple[str, ...]]]:
    """Read a snapshot, accepting UTF-8, UTF-8 BOM, or UTF-16 output."""
    raw = path.read_bytes()
    for encoding in ("utf-8-sig", "utf-16", "utf-16-le", "utf-16-be"):
        try:
            text = raw.decode(encoding)
            break
        except UnicodeDecodeError:
            continue
    else:
        raise ValueError(f"{path}: unsupported text encoding")

    rows = csv.reader(text.splitlines())
    try:
        header = tuple(next(rows))
    except StopIteration as exc:
        raise ValueError(f"{path}: empty CSV") from exc
    if header not in SUPPORTED_HEADERS:
        raise ValueError(
            f"{path}: unsupported header {header!r}; "
            f"supported headers are {sorted(SUPPORTED_HEADERS)!r}"
        )

    result: Counter[tuple[str, ...]] = Counter()
    for line_number, row in enumerate(rows, start=2):
        if not row or all(not value.strip() for value in row):
            continue
        if len(row) != len(header):
            raise ValueError(
                f"{path}:{line_number}: expected {len(header)} columns"
            )
        values = tuple(value.strip() for value in row)
        if any(not value for value in values):
            raise ValueError(f"{path}:{line_number}: empty field")
        result[values] += 1
    return header, result


def write_report(
    path: Path,
    header: tuple[str, ...],
    before: Counter,
    after: Counter,
) -> None:
    all_keys = sorted(set(before) | set(after))
    with path.open("w", encoding="utf-8", newline="") as stream:
        writer = csv.writer(stream)
        writer.writerow((*header, "change", "before_count", "after_count"))
        for key in all_keys:
            before_count = before.get(key, 0)
            after_count = after.get(key, 0)
            if before_count == after_count:
                change = "unchanged"
            elif before_count == 0:
                change = "added"
            elif after_count == 0:
                change = "removed"
            else:
                change = "count_changed"
            writer.writerow((*key, change, before_count, after_count))


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Compare two WmiInventory.exe CSV snapshots."
    )
    parser.add_argument("before", type=Path, help="baseline CSV")
    parser.add_argument("after", type=Path, help="comparison CSV")
    parser.add_argument(
        "-o", "--output", type=Path, help="write a complete comparison CSV"
    )
    parser.add_argument(
        "--fail-on-change",
        action="store_true",
        help="return 1 when any entry differs",
    )
    args = parser.parse_args()

    try:
        before_header, before = read_snapshot(args.before)
        after_header, after = read_snapshot(args.after)
        if before_header != after_header:
            raise ValueError(
                f"snapshot schemas differ: {before_header!r} vs {after_header!r}"
            )
        if args.output:
            write_report(args.output, before_header, before, after)
    except (OSError, ValueError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 2

    added = sorted(set(after) - set(before))
    removed = sorted(set(before) - set(after))
    changed = sorted(
        key for key in set(before) & set(after) if before[key] != after[key]
    )

    print(f"before={sum(before.values())} entries ({len(before)} unique)")
    print(f"after={sum(after.values())} entries ({len(after)} unique)")
    print(f"added={len(added)} removed={len(removed)} count_changed={len(changed)}")
    for values in added:
        print("ADDED\t" + "\t".join(values))
    for values in removed:
        print("REMOVED\t" + "\t".join(values))
    for values in changed:
        print(
            "COUNT_CHANGED\t"
            + "\t".join(values)
            + f"\t{before[values]}->{after[values]}"
        )

    if args.fail_on_change and (added or removed or changed):
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
