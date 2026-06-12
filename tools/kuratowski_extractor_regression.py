#!/usr/bin/env python3
"""NetworkX-backed regression for automatically extracted Kuratowski certificates.

For each simple graph, the C++ CLI returns PLANAR or a Kuratowski certificate
expressed as original edge IDs. NetworkX independently checks the planarity
decision, and this script suppresses degree-2 subdivision paths to verify that
each non-planar witness is exactly a K5 or K3,3 subdivision.
"""

from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path
from typing import Iterable, Iterator, Sequence

import networkx as nx

from differential_regression import (
    GraphCase,
    atlas_cases,
    batched,
    encode_batch,
    exhaustive_labeled_cases,
    random_cases,
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
    parser.add_argument("--batch-size", type=int, default=256)
    parser.add_argument("--no-atlas", action="store_true")
    return parser.parse_args()


def parse_cli_line(line: str) -> tuple[bool, str | None, tuple[int, ...]]:
    parts = line.split()
    if parts == ["PLANAR"]:
        return True, None, ()

    if len(parts) < 3 or parts[0] != "NONPLANAR" or parts[1] not in {"K5", "K33"}:
        raise ValueError(f"Unexpected certificate CLI output: {line!r}")

    edge_count = int(parts[2])
    edge_ids = tuple(int(value) for value in parts[3:])
    if edge_count != len(edge_ids):
        raise ValueError(
            f"Certificate declared {edge_count} edges but returned {len(edge_ids)}: {line!r}"
        )

    return False, parts[1], edge_ids


def run_batch(
    cli: Path,
    batch: Sequence[GraphCase],
) -> list[tuple[bool, str | None, tuple[int, ...]]]:
    process = subprocess.run(
        [str(cli)],
        input=encode_batch(batch),
        text=True,
        capture_output=True,
        check=False,
    )

    if process.returncode != 0:
        raise RuntimeError(
            f"C++ certificate CLI failed with exit code {process.returncode}.\n"
            f"stderr:\n{process.stderr}"
        )

    lines = [line.strip() for line in process.stdout.splitlines() if line.strip()]
    if len(lines) != len(batch):
        raise RuntimeError(
            f"Expected {len(batch)} CLI results, received {len(lines)}.\n"
            f"stdout:\n{process.stdout}\nstderr:\n{process.stderr}"
        )

    return [parse_cli_line(line) for line in lines]


def fail(case: GraphCase, message: str) -> AssertionError:
    return AssertionError(
        f"{case.name}: {message}\n"
        f"  vertices={case.vertex_count}\n"
        f"  edges={list(case.edges)}"
    )


def validate_certificate(
    case: GraphCase,
    declared_type: str,
    edge_ids: tuple[int, ...],
) -> None:
    if not edge_ids:
        raise fail(case, "non-planar result returned an empty certificate")
    if len(set(edge_ids)) != len(edge_ids):
        raise fail(case, f"certificate contains duplicate edge IDs: {edge_ids}")
    if any(edge_id < 0 or edge_id >= len(case.edges) for edge_id in edge_ids):
        raise fail(case, f"certificate contains an out-of-range edge ID: {edge_ids}")

    selected_edges = [case.edges[edge_id] for edge_id in edge_ids]
    certificate_graph = nx.Graph()
    certificate_graph.add_edges_from(selected_edges)

    if nx.check_planarity(certificate_graph, counterexample=False)[0]:
        raise fail(case, f"returned certificate is planar: {selected_edges}")

    branch_vertices = [
        vertex
        for vertex, degree in certificate_graph.degree()
        if degree != 2
    ]

    if any(degree not in {2, 3, 4} for _, degree in certificate_graph.degree()):
        raise fail(case, "certificate contains a vertex with degree outside {2, 3, 4}")

    edge_id_by_pair = {
        tuple(sorted(case.edges[edge_id])): edge_id
        for edge_id in edge_ids
    }
    visited_edge_ids: set[int] = set()
    kernel = nx.Graph()
    kernel.add_nodes_from(branch_vertices)
    branch_set = set(branch_vertices)

    for start in branch_vertices:
        for neighbor in certificate_graph.neighbors(start):
            current_edge_id = edge_id_by_pair[tuple(sorted((start, neighbor)))]
            if current_edge_id in visited_edge_ids:
                continue

            previous = start
            current = neighbor

            while True:
                if current_edge_id in visited_edge_ids:
                    raise fail(case, "two suppressed kernel paths reuse an original edge")
                visited_edge_ids.add(current_edge_id)

                if current in branch_set:
                    if current == start or kernel.has_edge(start, current):
                        raise fail(case, "suppression produced a loop or duplicate kernel edge")
                    kernel.add_edge(start, current)
                    break

                neighbors = list(certificate_graph.neighbors(current))
                if len(neighbors) != 2:
                    raise fail(case, "internal subdivision vertex does not have degree 2")

                next_vertex = neighbors[1] if neighbors[0] == previous else neighbors[0]
                previous, current = current, next_vertex
                current_edge_id = edge_id_by_pair[tuple(sorted((previous, current)))]

    if visited_edge_ids != set(edge_ids):
        raise fail(case, "certificate contains edges outside suppressed kernel paths")

    expected = nx.complete_graph(5) if declared_type == "K5" else nx.complete_bipartite_graph(3, 3)
    if not nx.is_isomorphic(kernel, expected):
        raise fail(
            case,
            f"suppressed kernel is not {declared_type}: kernel_edges={sorted(kernel.edges())}",
        )


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
        raise SystemExit(f"Certificate CLI does not exist: {args.cli}")

    sources: list[Iterable[GraphCase]] = []
    if include_atlas:
        sources.append(atlas_cases())
    sources.append(exhaustive_labeled_cases(exhaustive_max_n))
    sources.append(random_cases(random_count, args.random_max_n, args.seed))

    total = 0
    certificate_count = 0

    for source in sources:
        for batch in batched(source, args.batch_size):
            results = run_batch(args.cli, batch)

            for case, (actual_planar, declared_type, edge_ids) in zip(batch, results, strict=True):
                total += 1
                if actual_planar != case.expected_planar:
                    raise fail(
                        case,
                        f"decision mismatch: expected {case.expected_planar}, got {actual_planar}",
                    )

                if actual_planar:
                    continue

                assert declared_type is not None
                validate_certificate(case, declared_type, edge_ids)
                certificate_count += 1

            if total and total % 5000 < len(batch):
                print(f"Checked {total} graphs...", flush=True)

    print(f"Checked {total} graphs.")
    print(f"Validated {certificate_count} extracted Kuratowski certificates.")
    print("Kuratowski extractor regression passed: 0 failures.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (AssertionError, RuntimeError, ValueError) as error:
        print(error, file=sys.stderr)
        raise SystemExit(1)
