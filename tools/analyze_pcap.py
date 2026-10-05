#!/usr/bin/env python3
"""Analyse Duratech Dura-H17i BLE captures (Wireshark .pcapng) with tshark."""

from __future__ import annotations

import argparse
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path


def find_tshark() -> str:
    env = os.environ.get("TSHARK")
    if env and Path(env).is_file():
        return env
    found = shutil.which("tshark")
    if found:
        return found
    for candidate in (
        r"C:\Program Files\Wireshark\tshark.exe",
        "/usr/bin/tshark",
        "/usr/local/bin/tshark",
        "/Applications/Wireshark.app/Contents/MacOS/tshark",
    ):
        if Path(candidate).is_file():
            return candidate
    raise FileNotFoundError(
        "tshark not found. Install Wireshark or set TSHARK to the tshark binary path."
    )


def read_varint(data: bytes, i: int) -> tuple[int, int]:
    v = s = 0
    while i < len(data):
        b = data[i]
        i += 1
        v |= (b & 0x7F) << s
        s += 7
        if not (b & 0x80):
            break
    return v, i


def analyze_notify_fields(tshark: str, pcap: Path) -> None:
    cmd = [
        tshark,
        "-r",
        str(pcap),
        "-Y",
        "btatt.opcode == 0x1b && btatt.handle == 0x000c",
        "-T",
        "fields",
        "-e",
        "frame.time_relative",
        "-e",
        "btatt.value",
    ]
    out = subprocess.check_output(cmd, text=True, errors="replace")
    print(f"\n=== {pcap.name}: first hiWaterTemperature notify ===")
    for line in out.strip().splitlines():
        if not line.strip():
            continue
        t, hexv = line.split("\t")
        raw = bytes.fromhex(hexv.replace(":", ""))
        if b"hiWaterTemperature" not in raw:
            continue
        print(f"t={t} len={len(raw)}")
        for name in (
            b"hiWaterTemperature",
            b"hiSetpoint",
            b"hiPowerInput",
            b"hiPower",
            b"hiAmbientTemperature",
            b"hiWaterOutTemperature",
            b"hiNoiseMode",
            b"hiNoiseState",
        ):
            pos = raw.find(name)
            if pos < 0:
                print(f"  {name.decode()}: MISSING")
                continue
            chunk = raw[pos : pos + len(name) + 40]
            print(f"  {name.decode()}: {chunk.hex()}")
            i = len(name)
            vals = []
            j = i
            while j < min(len(chunk), i + 30):
                tag = chunk[j]
                if tag in (0x08, 0x10, 0x18, 0x20, 0x28, 0x30):
                    j += 1
                    v, j = read_varint(chunk, j)
                    vals.append((hex(tag), v))
                elif tag == 0x22:
                    j += 1
                    if j < len(chunk):
                        ln = chunk[j]
                        j += 1 + ln
                else:
                    j += 1
            print(f"    parsed: {vals}")
        names = [m.group().decode() for m in re.finditer(rb"hi[A-Z][a-zA-Z0-9]+", raw)]
        print(f"  fields: {names}")
        print(f"  head: {raw[:24].hex()}")
        break


def list_interesting_fields(tshark: str, pcap: Path) -> None:
    cmd = [
        tshark,
        "-r",
        str(pcap),
        "-Y",
        "btatt.opcode == 0x1b",
        "-T",
        "fields",
        "-e",
        "btatt.value",
    ]
    out = subprocess.check_output(cmd, text=True, errors="replace")
    names: set[str] = set()
    for hexv in out.splitlines():
        if not hexv.strip():
            continue
        raw = bytes.fromhex(hexv.replace(":", ""))
        for m in re.finditer(rb"hi[A-Z][a-zA-Z0-9]{2,40}", raw):
            names.add(m.group().decode())
    interesting = [
        n
        for n in sorted(names)
        if any(x in n.lower() for x in ("temp", "set", "power", "ambient", "water", "noise", "mode"))
    ]
    print(f"{pcap.name}: {', '.join(interesting) if interesting else '(none)'}")


def sequence_around_handshake(tshark: str, pcap: Path) -> None:
    cmd_w = [
        tshark,
        "-r",
        str(pcap),
        "-Y",
        "btatt.opcode == 0x12 && btatt.handle == 0x000f",
        "-T",
        "fields",
        "-e",
        "frame.time_relative",
        "-e",
        "btatt.value",
    ]
    cmd_n = [
        tshark,
        "-r",
        str(pcap),
        "-Y",
        "btatt.opcode == 0x1b && btatt.handle == 0x000c",
        "-T",
        "fields",
        "-e",
        "frame.time_relative",
        "-e",
        "btatt.value",
    ]
    writes: list[tuple[float, str, str]] = []
    for line in subprocess.check_output(cmd_w, text=True).splitlines():
        if not line.strip():
            continue
        t, v = line.split("\t")
        writes.append((float(t), "W", v.replace(":", "")))
    notifies: list[tuple[float, str, str]] = []
    for line in subprocess.check_output(cmd_n, text=True).splitlines():
        if not line.strip():
            continue
        t, v = line.split("\t")
        raw = bytes.fromhex(v.replace(":", ""))
        labels = []
        for name in (
            b"hiWaterTemperature",
            b"hiSetpoint",
            b"hiPowerInput",
            b"hiPower",
            b"hiGroupDashboard",
            b"hiHeatpumpState",
        ):
            if name in raw:
                labels.append(name.decode())
        notifies.append((float(t), "N", f"len={len(raw)} " + ",".join(labels[:6])))

    start = None
    for t, _, v in writes:
        if v.startswith("020a"):
            start = t
            break
    if start is None:
        print(f"{pcap.name}: no CONNECT")
        return
    events = [(t, d, p) for t, d, p in writes + notifies if start - 0.1 <= t <= start + 3.0]
    events.sort()
    print(f"\n=== {pcap.name}: +/-3s around first CONNECT @ {start:.3f} ===")
    for t, d, p in events:
        print(f"{t - start:+7.3f}s  {d}  {p}")


def collect_pcaps(path: Path) -> list[Path]:
    if path.is_file():
        return [path]
    if path.is_dir():
        return sorted(path.glob("*.pcapng")) + sorted(path.glob("*.pcap"))
    raise FileNotFoundError(path)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "path",
        nargs="?",
        default="captures",
        help="pcapng file or directory (default: captures/)",
    )
    parser.add_argument(
        "--handshake",
        action="store_true",
        help="show write/notify sequence around first CONNECT",
    )
    parser.add_argument(
        "--notify",
        action="store_true",
        help="decode first hiWaterTemperature notify in detail",
    )
    args = parser.parse_args()

    try:
        tshark = find_tshark()
    except FileNotFoundError as exc:
        print(exc, file=sys.stderr)
        return 1

    root = Path(__file__).resolve().parents[1]
    target = Path(args.path)
    if not target.is_absolute():
        target = root / target

    try:
        pcaps = collect_pcaps(target)
    except FileNotFoundError:
        print(f"No captures at {target}", file=sys.stderr)
        print("Place .pcapng files in captures/ (see captures/README.md)", file=sys.stderr)
        return 1

    if not pcaps:
        print(f"No .pcapng/.pcap files in {target}", file=sys.stderr)
        return 1

    print("=== interesting fields per pcap ===")
    for pcap in pcaps:
        try:
            list_interesting_fields(tshark, pcap)
        except subprocess.CalledProcessError as exc:
            print(f"{pcap.name}: tshark failed ({exc.returncode})")

    if args.handshake or args.notify:
        for pcap in pcaps:
            if args.notify:
                analyze_notify_fields(tshark, pcap)
            if args.handshake:
                sequence_around_handshake(tshark, pcap)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
