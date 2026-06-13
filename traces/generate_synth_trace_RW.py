#!/usr/bin/env python3

from __future__ import annotations

from pathlib import Path


NUM_CORES = 4
TOTAL_REQUESTS = 400_000  # reads + writes combined (50/50)
PAIRS_PER_CORE = TOTAL_REQUESTS // (2 * NUM_CORES)
MEM_SIZE_BYTES = 1 << 33  # 8 GiB
CACHE_LINE_BYTES = 64
OUTPUT_FILE = "synth_4_core_trace_rw.txt"
ISSUE_TIME = 0

# Match cache hierarchy: 16 KiB 8-way L1, 2 MiB 16-way LLC, 64 B lines.
L1_SIZE_BYTES = 16 * 1024
LLC_SIZE_BYTES = 2 * 1024 * 1024

# Mid-distance reuse: bigger than L1, much smaller than LLC.
# 512 lines = 32 KiB/core -> reliable L1 misses, strong LLC reuse.
WORKING_SET_LINES = 512

# Separate per-core regions to avoid false sharing; still fits easily in LLC.
CORE_REGION_BYTES = 256 * 1024
CORE_REGION_STRIDE = 1 << 24  # 16 MiB apart


def build_reuse_pool(core_id: int) -> list[int]:
    base = core_id * CORE_REGION_STRIDE
    pool = [
        base + line_index * CACHE_LINE_BYTES
        for line_index in range(WORKING_SET_LINES)
    ]

    for address in pool:
        if not (0 <= address < MEM_SIZE_BYTES):
            raise ValueError(f"Address 0x{address:x} exceeds the memory limit")

    region_end = base + WORKING_SET_LINES * CACHE_LINE_BYTES
    if region_end > base + CORE_REGION_BYTES:
        raise ValueError("Working set exceeds per-core region")

    total_unique_bytes = NUM_CORES * WORKING_SET_LINES * CACHE_LINE_BYTES
    if total_unique_bytes > LLC_SIZE_BYTES:
        raise ValueError(
            f"Total working set {total_unique_bytes} B exceeds LLC "
            f"{LLC_SIZE_BYTES} B; LLC misses will dominate"
        )

    return pool


def generate_trace() -> Path:
    output_path = Path(__file__).resolve().parent / OUTPUT_FILE
    address_pools = [build_reuse_pool(core_id) for core_id in range(NUM_CORES)]

    with output_path.open("w", encoding="utf-8") as trace_file:
        for pair_index in range(PAIRS_PER_CORE):
            for core_id, address_pool in enumerate(address_pools):
                address = address_pool[pair_index % len(address_pool)]
                trace_file.write(f"{ISSUE_TIME} {core_id} R {address:x}\n")
                trace_file.write(f"{ISSUE_TIME} {core_id} W {address:x}\n")

    return output_path


def main() -> None:
    output_path = generate_trace()
    print(f"Wrote {TOTAL_REQUESTS} requests to {output_path}")


if __name__ == "__main__":
    main()
