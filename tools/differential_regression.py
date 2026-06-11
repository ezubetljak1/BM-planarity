#!/usr/bin/env python3
"""Differential regression harness for the C++ BM planarity decision core.

The C++ executable receives batches of simple undirected graphs over stdin.
For every graph, its PLANAR/NONPLANAR decision is compared with NetworkX.
"""

from __future__ import annotations

import argparse
import itertools
import random
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable, Iterator, Sequence

try:
    import networkx as nx
except ImportError as exc:  # pragma: no cover - helpful CLI error
    raise SystemExit(
        "NetworkX is required. Install dev dependencies with: "
        "python -m pip install -r tools/requirements.txt"
    ) from exc


@dataclass(frozen=True)
class GraphCase:
    name: str
    vertex_count: int
    edges: tuple[tuple[int, int], ...]
    expected_planar: bool


def normalized_case(name: str, graph: nx.Graph) -> GraphCase:
    graph = nx.convert_node_labels_to_integers(nx.Graph(graph), ordering="sorted")
    edges = tuple(sorted((min(u, v), max(u, v)) for u, v in graph.edges()))
    expected_planar, _ = nx.check_planarity(graph, counterexample=False)
    return GraphCase(name, graph.number_of_nodes(), edges, expected_planar)


def atlas_cases() -> Iterator[GraphCase]:
    for index, graph in enumerate(nx.graph_atlas_g()):
        yield normalized_case(f"atlas:{index}", graph)


def exhaustive_labeled_cases(max_n: int) -> Iterator[GraphCase]:
    for vertex_count in range(max_n + 1):
        possible_edges = list(itertools.combinations(range(vertex_count), 2))

        for mask in range(1 << len(possible_edges)):
            graph = nx.Graph()
            graph.add_nodes_from(range(vertex_count))
            graph.add_edges_from(
                edge
                for bit_index, edge in enumerate(possible_edges)
                if mask & (1 << bit_index)
            )
            yield normalized_case(f"exhaustive:n={vertex_count}:mask={mask}", graph)


def random_cases(count: int, max_n: int, seed: int) -> Iterator[GraphCase]:
    rng = random.Random(seed)
    probabilities = (0.0, 0.02, 0.05, 0.1, 0.2, 0.35, 0.5, 0.75, 0.9, 1.0)

    for index in range(count):
        vertex_count = rng.randint(0, max_n)
        probability = rng.choice(probabilities)
        graph_seed = rng.randrange(0, 2**32)
        graph = nx.gnp_random_graph(vertex_count, probability, seed=graph_seed)
        yield normalized_case(
            f"random:{index}:n={vertex_count}:p={probability}:seed={graph_seed}",
            graph,
        )


def batched(cases: Iterable[GraphCase], batch_size: int) -> Iterator[list[GraphCase]]:
    batch: list[GraphCase] = []

    for case in cases:
        batch.append(case)
        if len(batch) == batch_size:
            yield batch
            batch = []

    if batch:
        yield batch


def encode_batch(batch: Sequence[GraphCase]) -> str:
    lines = [str(len(batch))]
    for case in batch:
        lines.append(f"{case.vertex_count} {len(case.edges)}")
        lines.extend(f"{u} {v}" for u, v in case.edges)
    return "\n".join(lines) + "\n"


def run_batch(cli: Path, batch: Sequence[GraphCase]) -> list[bool]:
    process = subprocess.run(
        [str(cli)],
        input=encode_batch(batch),
        text=True,
        capture_output=True,
        check=False,
    )

    if process.returncode != 0:
        raise RuntimeError(
            f"C++ decision CLI failed with exit code {process.returncode}.\n"
            f"stderr:\n{process.stderr}"
        )

    lines = [line.strip() for line in process.stdout.splitlines() if line.strip()]
    if len(lines) != len(batch):
        raise RuntimeError(
            f"Expected {len(batch)} CLI decisions, received {len(lines)}.\n"
            f"stdout:\n{process.stdout}\nstderr:\n{process.stderr}"
        )

    decisions: list[bool] = []
    for line in lines:
        if line == "1":
            decisions.append(True)
        elif line == "0":
            decisions.append(False)
        else:
            raise RuntimeError(f"Unexpected CLI output line: {line!r}")

    return decisions


def format_case(case: GraphCase) -> str:
    edge_text = ", ".join(f"({u}, {v})" for u, v in case.edges)
    return (
        f"{case.name}\n"
        f"  vertices={case.vertex_count}\n"
        f"  edges=[{edge_text}]\n"
        f"  expected={'PLANAR' if case.expected_planar else 'NONPLANAR'}"
    )


def profile_defaults(profile: str) -> tuple[int, int, bool]:
    if profile == "quick":
        return 5, 500, True
    if profile == "full":
        return 6, 5000, True
    raise ValueError(f"Unknown profile: {profile}")


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
    parser.add_argument("--max-reported-mismatches", type=int, default=10)
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
        raise SystemExit(f"Decision CLI does not exist: {args.cli}")
    if exhaustive_max_n < 0 or random_count < 0 or args.random_max_n < 0:
        raise SystemExit("Graph counts and limits must be non-negative.")
    if args.batch_size <= 0:
        raise SystemExit("Batch size must be positive.")

    sources: list[Iterable[GraphCase]] = []
    if include_atlas:
        sources.append(atlas_cases())
    sources.append(exhaustive_labeled_cases(exhaustive_max_n))
    sources.append(random_cases(random_count, args.random_max_n, args.seed))

    total = 0
    mismatches: list[str] = []

    for source in sources:
        for batch in batched(source, args.batch_size):
            actual_decisions = run_batch(args.cli, batch)

            for case, actual_planar in zip(batch, actual_decisions, strict=True):
                total += 1
                if actual_planar != case.expected_planar:
                    if len(mismatches) < args.max_reported_mismatches:
                        mismatches.append(
                            format_case(case)
                            + f"\n  actual={'PLANAR' if actual_planar else 'NONPLANAR'}"
                        )

            if total and total % 5000 < len(batch):
                print(f"Checked {total} graphs...", flush=True)

    print(f"Checked {total} graphs.")

    if mismatches:
        print(f"Found mismatches (showing {len(mismatches)}):", file=sys.stderr)
        for mismatch in mismatches:
            print("---", file=sys.stderr)
            print(mismatch, file=sys.stderr)
        return 1

    print("Differential regression passed: 0 mismatches.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
