#!/usr/bin/env python3

from __future__ import annotations

import bisect
import random
from dataclasses import dataclass, field
from pathlib import Path


NUM_CORES = 4
TOTAL_REQUESTS = 400_000
REQUESTS_PER_CORE = TOTAL_REQUESTS // NUM_CORES
TICK_INTERVAL = 1000
MEM_SIZE_BYTES = 1 << 33  # 1 GiB
CACHE_LINE_BYTES = 64
OUTPUT_FILE = "synth_4_core_trace.txt"
SEED = 20260425

# Access mix: frequent short- and mid-distance reuse, some long-distance reuse,
# some one-shot random traffic, and a small stream of new reusable addresses.
SHORT_REUSE_WEIGHT = 0.38
MID_REUSE_WEIGHT = 0.24
LONG_REUSE_WEIGHT = 0.10
NON_REUSE_WEIGHT = 0.18
NEW_REUSABLE_WEIGHT = 0.10
WRITE_PROBABILITY = 0.35

# Reuse windows are expressed in requests issued by the same core.
SHORT_REUSE_DISTANCE = (1, 8)
MID_REUSE_DISTANCE = (32, 256)
LONG_REUSE_DISTANCE = (1024, 4096)


@dataclass
class CoreRegions:
    hot: tuple[int, int]
    warm: tuple[int, int]
    cold: tuple[int, int]
    stream: tuple[int, int]


@dataclass
class CoreState:
    core_id: int
    rng: random.Random
    regions: CoreRegions
    history: list[int] = field(default_factory=list)
    reusable_positions: list[int] = field(default_factory=list)
    reusable_addresses: list[int] = field(default_factory=list)
    used_lines: set[int] = field(default_factory=set)

    def _alloc_unique_address(self, region: tuple[int, int]) -> int:
        start_addr, end_addr = region
        start_line = start_addr // CACHE_LINE_BYTES
        end_line = end_addr // CACHE_LINE_BYTES

        while True:
            line = self.rng.randrange(start_line, end_line)
            if line not in self.used_lines:
                self.used_lines.add(line)
                return line * CACHE_LINE_BYTES

    def _new_reusable_address(self) -> int:
        region_choices = [
            (self.regions.hot, 0.60),
            (self.regions.warm, 0.30),
            (self.regions.cold, 0.10),
        ]
        region = weighted_choice(self.rng, region_choices)
        return self._alloc_unique_address(region)

    def _new_non_reuse_address(self) -> int:
        return self._alloc_unique_address(self.regions.stream)

    def _pick_reuse_candidate(self, distance_range: tuple[int, int]) -> int | None:
        min_distance, max_distance = distance_range
        current_index = len(self.history)
        lower_index = max(0, current_index - max_distance)
        upper_index = current_index - min_distance
        if upper_index < 0:
            return None

        left = bisect.bisect_left(self.reusable_positions, lower_index)
        right = bisect.bisect_right(self.reusable_positions, upper_index)
        if left >= right:
            return None

        candidate = self.rng.randrange(left, right)
        return self.reusable_addresses[candidate]

    def next_request(self) -> tuple[str, int]:
        candidates: list[tuple[str, float]] = []

        if self._pick_reuse_candidate(SHORT_REUSE_DISTANCE) is not None:
            candidates.append(("short_reuse", SHORT_REUSE_WEIGHT))
        if self._pick_reuse_candidate(MID_REUSE_DISTANCE) is not None:
            candidates.append(("mid_reuse", MID_REUSE_WEIGHT))
        if self._pick_reuse_candidate(LONG_REUSE_DISTANCE) is not None:
            candidates.append(("long_reuse", LONG_REUSE_WEIGHT))

        candidates.append(("non_reuse", NON_REUSE_WEIGHT))
        candidates.append(("new_reusable", NEW_REUSABLE_WEIGHT))

        request_kind = weighted_choice(self.rng, candidates)
        if request_kind == "short_reuse":
            address = self._pick_reuse_candidate(SHORT_REUSE_DISTANCE)
            reusable = True
        elif request_kind == "mid_reuse":
            address = self._pick_reuse_candidate(MID_REUSE_DISTANCE)
            reusable = True
        elif request_kind == "long_reuse":
            address = self._pick_reuse_candidate(LONG_REUSE_DISTANCE)
            reusable = True
        elif request_kind == "new_reusable":
            address = self._new_reusable_address()
            reusable = True
        else:
            address = self._new_non_reuse_address()
            reusable = False

        assert address is not None
        self._record_access(address, reusable)

        command = "W" if self.rng.random() < WRITE_PROBABILITY else "R"
        return command, address

    def _record_access(self, address: int, reusable: bool) -> None:
        position = len(self.history)
        self.history.append(address)
        if reusable:
            self.reusable_positions.append(position)
            self.reusable_addresses.append(address)


def weighted_choice(rng: random.Random, weighted_items: list[tuple[object, float]]) -> object:
    total_weight = sum(weight for _, weight in weighted_items)
    pick = rng.uniform(0.0, total_weight)
    running_weight = 0.0
    for item, weight in weighted_items:
        running_weight += weight
        if pick <= running_weight:
            return item
    return weighted_items[-1][0]


def build_core_regions() -> list[CoreRegions]:
    per_core_bytes = MEM_SIZE_BYTES // NUM_CORES
    hot_bytes = 512 * 1024
    warm_bytes = 8 * 1024 * 1024
    cold_bytes = 64 * 1024 * 1024

    regions = []
    for core_id in range(NUM_CORES):
        base = core_id * per_core_bytes
        hot = (base, base + hot_bytes)
        warm = (hot[1], hot[1] + warm_bytes)
        cold = (warm[1], warm[1] + cold_bytes)
        stream = (cold[1], base + per_core_bytes)
        regions.append(CoreRegions(hot=hot, warm=warm, cold=cold, stream=stream))

    return regions


def generate_trace() -> Path:
    output_path = Path(__file__).resolve().parent / OUTPUT_FILE
    core_regions = build_core_regions()
    core_states = [
        CoreState(
            core_id=core_id,
            rng=random.Random(SEED + core_id),
            regions=core_regions[core_id],
        )
        for core_id in range(NUM_CORES)
    ]

    with output_path.open("w", encoding="utf-8") as trace_file:
        for request_index in range(REQUESTS_PER_CORE):
            timestamp = (request_index + 1) * TICK_INTERVAL
            for core in core_states:
                command, address = core.next_request()
                if not (0 <= address < MEM_SIZE_BYTES):
                    raise ValueError(f"Address 0x{address:x} exceeds the 1 GiB limit")
                trace_file.write(f"{timestamp} {core.core_id} {command} {address:x}\n")

    return output_path


def main() -> None:
    output_path = generate_trace()
    print(f"Wrote {TOTAL_REQUESTS} requests to {output_path}")


if __name__ == "__main__":
    main()
