#!/usr/bin/env python3
"""Package a flat, bank-1-linked C28 binary; never accepts a standalone .out.

Input octets must serialize each C28 word low octet first. Verify the hex2000
map/export before using this tool. This package is NOT STM32 CRC compatible.
"""
import argparse
import struct
import zlib
from pathlib import Path

CAPACITY = (0x20000 - 2 * 0x400) * 2
WIRE_SIZE = CAPACITY + 32


def pack(payload, board="cc"):
    target = {"cc": 0xF28A6501, "lp": 0xF28A6502}[board]
    if len(payload) < 4 or len(payload) > CAPACITY or len(payload) % 2:
        raise ValueError(f"payload must be even-sized, 4..{CAPACITY} octets")
    if payload[:4] in (b"\xff" * 4, b"\0" * 4):
        raise ValueError("blank codestart")
    header = struct.pack("<7I", 0x46323850, 1, target, len(payload),
                         0x0A0000, zlib.crc32(payload), 0)
    return (payload.ljust(CAPACITY, b"\xff") + header +
            struct.pack("<I", zlib.crc32(header)))


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("input", type=Path)
    ap.add_argument("output", type=Path)
    ap.add_argument("--board", choices=["cc", "lp"], default="cc")
    args = ap.parse_args()
    if args.input.resolve() == args.output.resolve():
        ap.error("input and output must differ")
    args.output.write_bytes(pack(args.input.read_bytes(), args.board))
    print(f"{args.output}: {WIRE_SIZE} wire octets; FoE password 0xF280")


if __name__ == "__main__":
    main()
