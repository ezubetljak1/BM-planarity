#!/usr/bin/env python3
"""Validate recovered C++ planar embeddings with NetworkX.

For every generated simple undirected graph, the C++ executable returns either
NONPLANAR or a cyclic edge order around each vertex. For planar graphs this
script converts the recovered rotation system into networkx.PlanarEmbedding and
runs check_structure().
"""

from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path
from typing import Sequence

import networkx as nx

from differential_regression import (
    GraphCase,
    atlas_cases,
    batched,
    encode_batch,
    exhaustive_labeled_cases,
    format_case,
    profile_defaults,
    random_cases,
)


def parse_embedding_line(case: GraphCase, line: str) -> tuple[bool, list[list[int]] | None]:
    tokens = line.split()
    if not tokens:
        raise RuntimeError("Embedding CLI returned an empty output line.")

    if tokens[0] == "0":
        if len(tokens) != 1:
            raise RuntimeError(f"Unexpected NONPLANAR output: {line!r}")
        return False, None

    if tokens[0] != "1":
        raise RuntimeError(f"Unexpected embedding CLI output: {line!r}")

    cursor = 1
    if cursor >= len(tokens):
        raise RuntimeError(f"Missing vertex count in embedding output: {line!r}")

    vertex_count = int(tokens[cursor])
    cursor += 1

    if vertex_count != case.vertex_count:
        raise RuntimeError(
            f"Embedding CLI returned vertex_count={vertex_count}, expected {case.vertex_count}."
        )

    rotations: list[list[int]] = []

    for _ in range(vertex_count):
        if cursor >= len(tokens):
            raise RuntimeError(f"Missing rotation degree in output: {line!r}")

        degree = int(tokens[cursor])
        cursor += 1

        if degree < 0 or cursor + degree > len(tokens):
            raise RuntimeError(f"Invalid rotation degree in output: {line!r}")

        rotation = [int(token) for token in tokens[cursor : cursor + degree]]
        cursor += degree
        rotations.append(rotation)

    if cursor != len(tokens):
        raise RuntimeError(f"Unexpected trailing tokens in embedding output: {line!r}")

    return True, rotations


def run_batch(cli: Path, batch: Sequence[GraphCase]) -> list[tuple[bool, list[list[int]] | None]]:
    process = subprocess.run(
        [str(cli)],
        input=encode_batch(batch),
        text=True,
        capture_output=True,
        check=False,
    )

    if process.returncode != 0:
        raise RuntimeError(
            f"C++ embedding CLI failed with exit code {process.returncode}.\n"
            f"stderr:\n{process.stderr}"
        )

    lines = [line.strip() for line in process.stdout.splitlines() if line.strip()]
    if len(lines) != len(batch):
        raise RuntimeError(
            f"Expected {len(batch)} CLI outputs, received {len(lines)}.\n"
            f"stdout:\n{process.stdout}\nstderr:\n{process.stderr}"
        )

    return [parse_embedding_line(case, line) for case, line in zip(batch, lines, strict=True)]


def edge_other_endpoint(case: GraphCase, edge_id: int, vertex: int) -> int:
    if edge_id < 0 or edge_id >= len(case.edges):
        raise RuntimeError(f"Invalid edge id {edge_id} in recovered embedding.")

    u, v = case.edges[edge_id]
    if u == vertex:
        return v
    if v == vertex:
        return u
    raise RuntimeError(f"Edge id {edge_id} is not incident to vertex {vertex}.")


def validate_networkx_embedding(case: GraphCase, rotations: list[list[int]]) -> None:
    if len(rotations) != case.vertex_count:
        raise RuntimeError("Recovered embedding has the wrong number of vertex rotations.")

    data: dict[int, list[int]] = {}
    for vertex, rotation in enumerate(rotations):
        data[vertex] = [edge_other_endpoint(case, edge_id, vertex) for edge_id in rotation]

    embedding = nx.PlanarEmbedding()
    embedding.set_data(data)
    embedding.check_structure()


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--cli", required=True, type=Path)
    parser.add_argument("--profile", choices=("quick", "full"), default="quick")
    parser.add_argument("--exhaustive-max-n", type=int)
    parser.add_argument("--random-count", type=int)
    parser.add_argument("--random-max-n", type=int, default=30)
    parser.add_argument("--seed", type=int, default=19676)
    parser.add_argument("--batch-size", type=int, default=512)
    parser.add_argument("--no-atlas", action="store_true")
    parser.add_argument("--max-reported-failures", type=int, default=10)
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    exhaustive_max_n, random_count, include_atlas = profile_defaults(args.profile)

    if args.exhaustive_max_n is not None:
        exhaustive_max_n = args.exhaustive_max_n
    if args.random_count is not None:
        random_count = args.random_count
    if args.no_atlas:
        include_atlas = False

    if not args.cli.exists():
        raise SystemExit(f"Embedding CLI does not exist: {args.cli}")
    if exhaustive_max_n < 0 or random_count < 0 or args.random_max_n < 0:
        raise SystemExit("Graph counts and limits must be non-negative.")
    if args.batch_size <= 0:
        raise SystemExit("Batch size must be positive.")

    sources = []
    if include_atlas:
        sources.append(atlas_cases())
    sources.append(exhaustive_labeled_cases(exhaustive_max_n))
    sources.append(random_cases(random_count, args.random_max_n, args.seed))

    total = 0
    planar_checked = 0
    failures: list[str] = []

    for source in sources:
        for batch in batched(source, args.batch_size):
            actual = run_batch(args.cli, batch)

            for case, (actual_planar, rotations) in zip(batch, actual, strict=True):
                total += 1

                if actual_planar != case.expected_planar:
                    if len(failures) < args.max_reported_failures:
                        failures.append(
                            format_case(case)
                            + f"\n  actual={'PLANAR' if actual_planar else 'NONPLANAR'}"
                        )
                    continue

                if actual_planar:
                    try:
                        assert rotations is not None
                        validate_networkx_embedding(case, rotations)
                        planar_checked += 1
                    except Exception as exc:  # pragma: no cover - diagnostic path
                        if len(failures) < args.max_reported_failures:
                            failures.append(format_case(case) + f"\n  embedding_error={exc}")

            if total and total % 5000 < len(batch):
                print(f"Checked {total} graphs...", flush=True)

    print(f"Checked {total} graphs.")
    print(f"Validated {planar_checked} recovered planar embeddings with NetworkX.")

    if failures:
        print(f"Found failures (showing {len(failures)}):", file=sys.stderr)
        for failure in failures:
            print("---", file=sys.stderr)
            print(failure, file=sys.stderr)
        return 1

    print("Embedding regression passed: 0 failures.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
