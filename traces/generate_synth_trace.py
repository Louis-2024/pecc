#!/usr/bin/env python3

from __future__ import annotations

from pathlib import Path


NUM_CORES = 4
TOTAL_REQUESTS = 400_000
REQUESTS_PER_CORE = TOTAL_REQUESTS // NUM_CORES
MEM_SIZE_BYTES = 1 << 33  # 8 GiB
CACHE_LINE_BYTES = 64
OUTPUT_FILE = "synth_4_core_trace.txt"
ISSUE_TIME = 0
COMMAND = "R"

# These match the run_synth.py Ruby configuration:
# 64B lines, 16 KiB 2-way L1s, 1 MiB 8-way LLC split into 8 banks.
L1_SET_BITS = 7
L1_SET_MASK = (1 << L1_SET_BITS) - 1
TARGET_L1_SET = 0

# Bits 13-16 vary the LLC set while keeping the L1 set fixed. With 32 lines per
# core and 4 cores, each touched LLC set gets exactly 8 lines, matching LLC assoc.
LINES_PER_CORE_POOL = 32
CORE_ADDRESS_STRIDE = 1 << 30


def l1_set(address: int) -> int:
    return (address // CACHE_LINE_BYTES) & L1_SET_MASK


def build_l1_conflict_pool(core_id: int) -> list[int]:
    core_base = core_id * CORE_ADDRESS_STRIDE
    addresses = [
        core_base | (line_index << 13) | (TARGET_L1_SET << 6)
        for line_index in range(LINES_PER_CORE_POOL)
    ]

    for address in addresses:
        if not (0 <= address < MEM_SIZE_BYTES):
            raise ValueError(f"Address 0x{address:x} exceeds the memory limit")
        if l1_set(address) != TARGET_L1_SET:
            raise ValueError(
                f"Address 0x{address:x} maps to L1 set {l1_set(address)}, "
                f"expected {TARGET_L1_SET}"
            )

    return addresses


def generate_trace() -> Path:
    output_path = Path(__file__).resolve().parent / OUTPUT_FILE
    address_pools = [build_l1_conflict_pool(core_id) for core_id in range(NUM_CORES)]

    with output_path.open("w", encoding="utf-8") as trace_file:
        for request_index in range(REQUESTS_PER_CORE):
            for core_id, address_pool in enumerate(address_pools):
                address = address_pool[request_index % len(address_pool)]
                trace_file.write(f"{ISSUE_TIME} {core_id} {COMMAND} {address:x}\n")

    return output_path


def main() -> None:
    output_path = generate_trace()
    print(f"Wrote {TOTAL_REQUESTS} requests to {output_path}")


if __name__ == "__main__":
    main()
