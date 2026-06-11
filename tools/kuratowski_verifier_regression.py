#!/usr/bin/env python3
"""Validate the C++ Kuratowski certificate verifier with NetworkX witnesses."""

from __future__ import annotations

import argparse
import random
import subprocess
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable, Sequence

import networkx as nx


@dataclass(frozen=True)
class CertificateCase:
    name: str
    vertex_count: int
    edges: tuple[tuple[int, int], ...]
    certificate_edge_ids: tuple[int, ...]


def normalized_edge(first: int, second: int) -> tuple[int, int]:
    return (first, second) if first < second else (second, first)


def networkx_certificate_case(name: str, graph: nx.Graph) -> CertificateCase | None:
    planar, witness = nx.check_planarity(graph, counterexample=True)
    if planar:
        return None

    vertices = sorted(graph.nodes())
    if vertices != list(range(len(vertices))):
        mapping = {vertex: index for index, vertex in enumerate(vertices)}
        graph = nx.relabel_nodes(graph, mapping, copy=True)
        witness = nx.relabel_nodes(witness, mapping, copy=True)

    edges = tuple(sorted(normalized_edge(first, second) for first, second in graph.edges()))
    edge_id_by_pair = {edge: index for index, edge in enumerate(edges)}

    certificate_edge_ids = tuple(
        sorted(edge_id_by_pair[normalized_edge(first, second)] for first, second in witness.edges())
    )

    return CertificateCase(name, graph.number_of_nodes(), edges, certificate_edge_ids)


def atlas_cases() -> list[CertificateCase]:
    cases: list[CertificateCase] = []
    for index, graph in enumerate(nx.graph_atlas_g()):
        case = networkx_certificate_case(f"atlas:{index}", graph)
        if case is not None:
            cases.append(case)
    return cases


def random_nonplanar_cases(count: int, max_n: int, seed: int) -> list[CertificateCase]:
    rng = random.Random(seed)
    cases: list[CertificateCase] = []
    attempt = 0

    while len(cases) < count:
        attempt += 1
        vertex_count = rng.randint(6, max(6, max_n))
        probability = rng.uniform(0.20, 0.85)
        graph = nx.gnp_random_graph(vertex_count, probability, seed=rng.randrange(1 << 30))
        case = networkx_certificate_case(f"random:{attempt}:n={vertex_count}:p={probability:.3f}", graph)
        if case is not None:
            cases.append(case)

    return cases


def encode_batch(cases: Sequence[CertificateCase]) -> str:
    lines = [str(len(cases))]

    for case in cases:
        lines.append(f"{case.vertex_count} {len(case.edges)}")
        lines.extend(f"{first} {second}" for first, second in case.edges)
        lines.append(str(len(case.certificate_edge_ids)))
        lines.append(" ".join(str(edge_id) for edge_id in case.certificate_edge_ids))

    return "\n".join(lines) + "\n"


def batched(items: Sequence[CertificateCase], batch_size: int) -> Iterable[Sequence[CertificateCase]]:
    for start in range(0, len(items), batch_size):
        yield items[start : start + batch_size]


def run_batch(cli: Path, cases: Sequence[CertificateCase]) -> None:
    process = subprocess.run(
        [str(cli)],
        input=encode_batch(cases),
        text=True,
        capture_output=True,
        check=False,
    )

    if process.returncode != 0:
        raise RuntimeError(
            f"C++ Kuratowski verifier CLI failed with exit code {process.returncode}.\n"
            f"stderr:\n{process.stderr}"
        )

    lines = [line.strip() for line in process.stdout.splitlines() if line.strip()]
    if len(lines) != len(cases):
        raise RuntimeError(
            f"Expected {len(cases)} verifier outputs, received {len(lines)}.\n"
            f"stdout:\n{process.stdout}\nstderr:\n{process.stderr}"
        )


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--cli", required=True, type=Path)
    parser.add_argument("--profile", choices=("quick", "full"), default="quick")
    parser.add_argument("--random-count", type=int)
    parser.add_argument("--random-max-n", type=int, default=30)
    parser.add_argument("--seed", type=int, default=19676)
    parser.add_argument("--batch-size", type=int, default=128)
    return parser.parse_args()


def main() -> int:
    args = parse_args()

    random_count = 100 if args.profile == "quick" else 300
    if args.random_count is not None:
        random_count = args.random_count

    if not args.cli.exists():
        raise SystemExit(f"Verifier CLI does not exist: {args.cli}")
    if random_count < 0 or args.random_max_n < 6:
        raise SystemExit("Random graph count must be non-negative and random-max-n must be at least 6.")
    if args.batch_size <= 0:
        raise SystemExit("Batch size must be positive.")

    cases = atlas_cases()
    cases.extend(random_nonplanar_cases(random_count, args.random_max_n, args.seed))

    for batch in batched(cases, args.batch_size):
        run_batch(args.cli, batch)

    print(f"Checked {len(cases)} NetworkX Kuratowski witnesses.")
    print("Kuratowski verifier regression passed: 0 failures.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
