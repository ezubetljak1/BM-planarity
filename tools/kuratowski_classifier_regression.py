#!/usr/bin/env python3
"""Differential smoke test for reference-style Kuratowski minor classification."""

from __future__ import annotations

import argparse
import random
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable

import networkx as nx


@dataclass(frozen=True)
class Case:
    label: str
    graph: nx.Graph


def normalized_graph(graph: nx.Graph) -> nx.Graph:
    result = nx.Graph()
    result.add_nodes_from(range(graph.number_of_nodes()))
    mapping = {vertex: index for index, vertex in enumerate(graph.nodes())}
    result.add_edges_from((mapping[u], mapping[v]) for u, v in graph.edges())
    return result


def graph_atlas_cases() -> Iterable[Case]:
    for index, graph in enumerate(nx.graph_atlas_g()):
        yield Case(f"atlas:{index}", normalized_graph(graph))


def exhaustive_cases(max_vertices: int) -> Iterable[Case]:
    for vertex_count in range(max_vertices + 1):
        edges = [
            (first, second)
            for first in range(vertex_count)
            for second in range(first + 1, vertex_count)
        ]

        for mask in range(1 << len(edges)):
            graph = nx.Graph()
            graph.add_nodes_from(range(vertex_count))
            graph.add_edges_from(
                edge for index, edge in enumerate(edges) if mask & (1 << index)
            )
            yield Case(f"exhaustive:n={vertex_count}:mask={mask}", graph)


def random_cases(count: int, seed: int, max_vertices: int) -> Iterable[Case]:
    generator = random.Random(seed)

    for index in range(count):
        vertex_count = generator.randint(0, max_vertices)
        probability = generator.random()
        graph_seed = generator.randint(0, 2**31 - 1)
        graph = nx.gnp_random_graph(vertex_count, probability, seed=graph_seed)
        yield Case(
            f"random:{index}:n={vertex_count}:p={probability:.6f}:seed={graph_seed}",
            graph,
        )


def cases_for_profile(profile: str) -> list[Case]:
    if profile == "quick":
        return [
            *graph_atlas_cases(),
            *exhaustive_cases(5),
            *random_cases(500, 19676, 24),
        ]

    return [
        *graph_atlas_cases(),
        *exhaustive_cases(6),
        *random_cases(5000, 19676, 40),
    ]


def encode_cases(cases: list[Case]) -> str:
    lines = [str(len(cases))]

    for case in cases:
        edges = list(case.graph.edges())
        lines.append(f"{case.graph.number_of_nodes()} {len(edges)}")
        lines.extend(f"{first} {second}" for first, second in edges)

    return "\n".join(lines) + "\n"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--cli", required=True, type=Path)
    parser.add_argument("--profile", choices=("quick", "full"), default="quick")
    args = parser.parse_args()

    cases = cases_for_profile(args.profile)
    completed = subprocess.run(
        [str(args.cli)],
        input=encode_cases(cases),
        text=True,
        capture_output=True,
        check=False,
    )

    if completed.returncode != 0:
        print(completed.stderr, file=sys.stderr)
        return completed.returncode

    lines = completed.stdout.splitlines()

    if len(lines) != len(cases):
        print(
            f"Expected {len(cases)} classifier outputs, received {len(lines)}.",
            file=sys.stderr,
        )
        return 1

    failures: list[str] = []
    classified = 0

    for case, line in zip(cases, lines):
        expected_planar = nx.check_planarity(case.graph, counterexample=False)[0]
        tokens = line.split()

        if expected_planar:
            if tokens != ["PLANAR"]:
                failures.append(f"{case.label}: expected PLANAR, got {line!r}")
            continue

        classified += 1

        if len(tokens) != 2 or tokens[0] != "NONPLANAR" or tokens[1] not in "ABCDE":
            failures.append(f"{case.label}: expected NONPLANAR A-E, got {line!r}")

    print(f"Checked {len(cases)} graphs.")
    print(f"Classified {classified} non-planar failures as A-E.")

    if failures:
        print(f"Kuratowski classifier regression failed: {len(failures)} issue(s).")
        for failure in failures[:20]:
            print(f"  {failure}")
        return 1

    print("Kuratowski classifier regression passed: 0 failures.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
