#!/usr/bin/env python3
"""Generate thesis-ready tables and plots from BM benchmark raw CSV data."""

from __future__ import annotations

import argparse
import csv
import math
import statistics
from collections import defaultdict
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable, Sequence

try:
    import matplotlib.pyplot as plt
except ImportError as exc:  # pragma: no cover
    raise SystemExit(
        "Matplotlib is required. Install benchmark dependencies with: "
        "python -m pip install -r tools/requirements-benchmark.txt"
    ) from exc


CM_TO_INCH = 1.0 / 2.54
PICTURE_WIDTH_CM = 16.0
HEIGHT_WIDTH_RATIO = 0.65


@dataclass(frozen=True)
class RawMeasurement:
    scenario_index: int
    family: str
    instance: int
    requested_n: int
    n: int
    m: int
    work_size: int
    expected_planarity: str
    actual_planarity: str
    repetition: int
    elapsed_ns: int
    ns_per_work_item: float
    seed: int


@dataclass(frozen=True)
class SummaryRow:
    family: str
    requested_n: int
    n: int
    m: int
    work_size: int
    expected_planarity: str
    actual_planarity: str
    sample_count: int
    instance_count: int
    median_ns: float
    q1_ns: float
    q3_ns: float
    min_ns: int
    max_ns: int
    median_ns_per_work_item: float


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", required=True, type=Path)
    parser.add_argument("--output-dir", required=True, type=Path)
    return parser.parse_args()


def configure_matplotlib() -> None:
    plt.rcParams.update(
        {
            "font.size": 10,
            "axes.labelsize": 11,
            "axes.titlesize": 11,
            "legend.fontsize": 8,
            "xtick.labelsize": 9,
            "ytick.labelsize": 9,
            "axes.spines.top": False,
            "axes.spines.right": False,
            "axes.grid": True,
            "grid.alpha": 0.25,
            "grid.linewidth": 0.6,
            "lines.linewidth": 1.5,
            "lines.markersize": 4.5,
            "savefig.dpi": 300,
        }
    )


def read_measurements(path: Path) -> list[RawMeasurement]:
    rows: list[RawMeasurement] = []
    with path.open(newline="", encoding="utf-8") as input_file:
        reader = csv.DictReader(input_file)
        for row in reader:
            rows.append(
                RawMeasurement(
                    scenario_index=int(row["scenario_index"]),
                    family=row["family"],
                    instance=int(row["instance"]),
                    requested_n=int(row["requested_n"]),
                    n=int(row["n"]),
                    m=int(row["m"]),
                    work_size=int(row["work_size"]),
                    expected_planarity=row["expected_planarity"],
                    actual_planarity=row["actual_planarity"],
                    repetition=int(row["repetition"]),
                    elapsed_ns=int(row["elapsed_ns"]),
                    ns_per_work_item=float(row["ns_per_work_item"]),
                    seed=int(row["seed"]),
                )
            )
    if not rows:
        raise ValueError(f"No measurements found in {path}")
    return rows


def quartiles(values: Sequence[float]) -> tuple[float, float]:
    if len(values) == 1:
        return values[0], values[0]
    cuts = statistics.quantiles(values, n=4, method="inclusive")
    return cuts[0], cuts[2]


def summarize(measurements: Iterable[RawMeasurement]) -> list[SummaryRow]:
    groups: dict[tuple[str, int], list[RawMeasurement]] = defaultdict(list)
    for measurement in measurements:
        groups[(measurement.family, measurement.requested_n)].append(measurement)

    summary: list[SummaryRow] = []
    for (family, requested_n), rows in sorted(groups.items()):
        elapsed = [row.elapsed_ns for row in rows]
        normalized = [row.ns_per_work_item for row in rows]
        q1, q3 = quartiles(elapsed)
        actual_values = sorted({row.actual_planarity for row in rows})
        if len(actual_values) != 1:
            raise ValueError(f"Inconsistent actual planarity in {family} n={requested_n}: {actual_values}")

        first = rows[0]
        summary.append(
            SummaryRow(
                family=family,
                requested_n=requested_n,
                n=first.n,
                m=first.m,
                work_size=first.work_size,
                expected_planarity=first.expected_planarity,
                actual_planarity=actual_values[0],
                sample_count=len(rows),
                instance_count=len({row.instance for row in rows}),
                median_ns=statistics.median(elapsed),
                q1_ns=q1,
                q3_ns=q3,
                min_ns=min(elapsed),
                max_ns=max(elapsed),
                median_ns_per_work_item=statistics.median(normalized),
            )
        )
    return summary


def write_summary_csv(path: Path, rows: Sequence[SummaryRow]) -> None:
    fieldnames = [field for field in SummaryRow.__dataclass_fields__]
    with path.open("w", newline="", encoding="utf-8") as output_file:
        writer = csv.DictWriter(output_file, fieldnames=fieldnames)
        writer.writeheader()
        for row in rows:
            writer.writerow({field: getattr(row, field) for field in fieldnames})


def family_label(family: str) -> str:
    labels = {
        "path": "Put",
        "cycle": "Ciklus",
        "wheel": "Točak",
        "grid": "Mreža",
        "stacked_triangulation": "Maksimalno planaran graf",
        "random_tree": "Nasumično stablo",
        "random_sparse": "Nasumični rijetki graf",
        "subdivided_k33": r"Potpodjela $K_{3,3}$",
        "subdivided_k5": r"Potpodjela $K_5$",
        "complete": r"Kompletan graf $K_n$",
    }
    return labels.get(family, family.replace("_", " "))


def group_by_family(rows: Sequence[SummaryRow]) -> dict[str, list[SummaryRow]]:
    result: dict[str, list[SummaryRow]] = defaultdict(list)
    for row in rows:
        result[row.family].append(row)
    for values in result.values():
        values.sort(key=lambda row: row.work_size)
    return dict(sorted(result.items()))


def save_figure(fig: plt.Figure, output_base: Path) -> None:
    output_base.parent.mkdir(parents=True, exist_ok=True)
    fig.set_size_inches(PICTURE_WIDTH_CM * CM_TO_INCH, PICTURE_WIDTH_CM * HEIGHT_WIDTH_RATIO * CM_TO_INCH)
    for suffix in (".png", ".pdf", ".svg"):
        fig.savefig(output_base.with_suffix(suffix), bbox_inches="tight")
    plt.close(fig)


def plot_runtime_vs_work(rows: Sequence[SummaryRow], output_dir: Path) -> None:
    fig, ax = plt.subplots()
    for family, values in group_by_family(rows).items():
        x = [row.work_size for row in values]
        y = [row.median_ns / 1_000_000.0 for row in values]
        lower = [row.q1_ns / 1_000_000.0 for row in values]
        upper = [row.q3_ns / 1_000_000.0 for row in values]
        line = ax.plot(x, y, marker="o", label=family_label(family))[0]
        ax.fill_between(x, lower, upper, alpha=0.14, color=line.get_color())

    ax.set_xscale("log")
    ax.set_yscale("log")
    ax.set_xlabel(r"Veličina ulaza $n + m$")
    ax.set_ylabel(r"Medijana vremena izvršavanja (ms)")
    ax.legend()
    save_figure(fig, output_dir / "runtime_vs_work_size")


def plot_normalized_runtime(rows: Sequence[SummaryRow], output_dir: Path) -> None:
    fig, ax = plt.subplots()
    for family, values in group_by_family(rows).items():
        x = [row.work_size for row in values]
        y = [row.median_ns_per_work_item for row in values]
        ax.plot(x, y, marker="o", label=family_label(family))

    ax.set_xscale("log")
    ax.set_xlabel(r"Veličina ulaza $n + m$")
    ax.set_ylabel(r"Vrijeme po elementu ulaza (ns / $(n + m)$)")
    ax.legend()
    save_figure(fig, output_dir / "normalized_runtime")


def plot_sparse_runtime_vs_n(rows: Sequence[SummaryRow], output_dir: Path) -> None:
    filtered = [row for row in rows if row.family != "complete"]
    fig, ax = plt.subplots()
    for family, values in group_by_family(filtered).items():
        x = [row.n for row in values]
        y = [row.median_ns / 1_000_000.0 for row in values]
        ax.plot(x, y, marker="o", label=family_label(family))

    ax.set_xscale("log")
    ax.set_yscale("log")
    ax.set_xlabel(r"Broj čvorova $n$")
    ax.set_ylabel(r"Medijana vremena izvršavanja (ms)")
    ax.legend()
    save_figure(fig, output_dir / "sparse_runtime_vs_n")


def plot_dense_complete(rows: Sequence[SummaryRow], output_dir: Path) -> None:
    complete = [row for row in rows if row.family == "complete"]
    if not complete:
        return

    fig, ax = plt.subplots()
    complete.sort(key=lambda row: row.m)
    ax.plot(
        [row.m for row in complete],
        [row.median_ns / 1_000_000.0 for row in complete],
        marker="o",
        label=r"Kompletan graf $K_n$",
    )
    ax.set_xscale("log")
    ax.set_yscale("log")
    ax.set_xlabel(r"Broj grana $m$")
    ax.set_ylabel(r"Medijana vremena izvršavanja (ms)")
    ax.legend()
    save_figure(fig, output_dir / "dense_complete_runtime_vs_m")


def log_log_slope(rows: Sequence[SummaryRow]) -> float | None:
    if len(rows) < 3:
        return None
    ordered = sorted(rows, key=lambda row: row.work_size)
    tail = ordered[len(ordered) // 2 :]
    x = [math.log(row.work_size) for row in tail if row.work_size > 0 and row.median_ns > 0]
    y = [math.log(row.median_ns) for row in tail if row.work_size > 0 and row.median_ns > 0]
    if len(x) < 2:
        return None
    mean_x = statistics.mean(x)
    mean_y = statistics.mean(y)
    denominator = sum((value - mean_x) ** 2 for value in x)
    if denominator == 0:
        return None
    return sum((x_value - mean_x) * (y_value - mean_y) for x_value, y_value in zip(x, y)) / denominator


def write_slopes_csv(path: Path, rows: Sequence[SummaryRow]) -> None:
    with path.open("w", newline="", encoding="utf-8") as output_file:
        writer = csv.writer(output_file)
        writer.writerow(["family", "tail_log_log_slope"])
        for family, values in group_by_family(rows).items():
            writer.writerow([family, log_log_slope(values)])


def write_markdown_report(path: Path, rows: Sequence[SummaryRow]) -> None:
    slopes = {family: log_log_slope(values) for family, values in group_by_family(rows).items()}
    lines = [
        "# Sažetak benchmark rezultata",
        "",
        "## Procijenjeni log-log nagibi na većim ulazima",
        "",
        "Nagib blizu 1 je empirijski kompatibilan sa linearnim rastom vremena u odnosu na veličinu ulaza `n + m`. Ovo nije formalni dokaz složenosti.",
        "",
        "| Familija | Procijenjeni nagib |",
        "|---|---:|",
    ]
    for family, slope in slopes.items():
        formatted = "—" if slope is None else f"{slope:.3f}"
        lines.append(f"| {family_label(family)} | {formatted} |")

    lines.extend(
        [
            "",
            "## Datoteke",
            "",
            "- `summary.csv`: medijana, kvartili i normalizovano vrijeme po familiji i veličini ulaza.",
            "- `scaling_slopes.csv`: procijenjeni log-log nagibi.",
            "- `*.png`, `*.pdf`, `*.svg`: figure spremne za uključivanje u rad.",
            "",
        ]
    )
    path.write_text("\n".join(lines), encoding="utf-8")


def main() -> int:
    args = parse_args()
    configure_matplotlib()
    args.output_dir.mkdir(parents=True, exist_ok=True)
    measurements = read_measurements(args.input)
    summary = summarize(measurements)
    write_summary_csv(args.output_dir / "summary.csv", summary)
    write_slopes_csv(args.output_dir / "scaling_slopes.csv", summary)
    write_markdown_report(args.output_dir / "README.md", summary)
    plot_runtime_vs_work(summary, args.output_dir)
    plot_normalized_runtime(summary, args.output_dir)
    plot_sparse_runtime_vs_n(summary, args.output_dir)
    plot_dense_complete(summary, args.output_dir)
    print(f"Generated benchmark figures and tables in: {args.output_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
