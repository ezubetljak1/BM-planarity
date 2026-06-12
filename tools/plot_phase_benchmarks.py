#!/usr/bin/env python3
"""Generate publication-quality phase-level diagnostic plots."""

from __future__ import annotations

import argparse
import csv
import statistics
from collections import defaultdict
from pathlib import Path
from typing import Iterable

import matplotlib.pyplot as plt

PICTURE_WIDTH_CM = 20.0
HEIGHT_WIDTH_RATIO = 0.65
CM_TO_INCH = 1.0 / 2.54

TOP_LEVEL_PHASES = (
    ("validation_ns", "Validacija ulaza"),
    ("dfs_preprocessing_ns", "DFS preprocessing"),
    ("state_initialization_ns", "Inicijalizacija stanja"),
    ("decision_core_ns", "Decision core"),
    ("failure_factory_ns", "Failure snapshot"),
    ("kuratowski_preparation_ns", "Priprema izolacije"),
    ("kuratowski_isolation_ns", "Izolacija minora"),
    ("certificate_verification_ns", "Verifikacija certifikata"),
    ("unaccounted_ns", "Preostali trošak"),
)

PREPARATION_PHASES = (
    ("kuratowski_oriented_state_copy_ns", "Kopiranje failure stanja"),
    ("kuratowski_orientation_ns", "Normalizacija orijentacije"),
    ("kuratowski_context_initialization_ns", "Inicijalizacija konteksta"),
    ("kuratowski_minor_classification_ns", "Klasifikacija minora"),
    ("preparation_remainder_ns", "Preostali trošak pripreme"),
)

CLASSIFICATION_PHASES = (
    ("kuratowski_classify_initial_ns", "Početna A/B klasifikacija"),
    ("kuratowski_classify_external_face_vertices_ns", "Označavanje spoljašnjeg lica"),
    ("kuratowski_find_highest_xy_path_ns", "Traženje najvišeg X-Y puta"),
    ("kuratowski_find_z_to_root_path_ns", "Traženje Z-R puta"),
    ("kuratowski_find_future_pertinent_below_xy_path_ns", "Traženje future-pertinent čvora"),
    ("classification_remainder_ns", "Preostali trošak klasifikacije"),
)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", required=True, type=Path)
    parser.add_argument("--output-dir", required=True, type=Path)
    return parser.parse_args()


def configure_matplotlib() -> None:
    plt.rcParams.update(
        {
            "font.size": 13,
            "axes.spines.top": False,
            "axes.spines.right": False,
            "axes.grid": True,
            "grid.alpha": 0.25,
            "lines.linewidth": 1.5,
            "legend.fontsize": 10,
            "savefig.dpi": 300,
        }
    )


def load_rows(path: Path) -> list[dict[str, str]]:
    with path.open(newline="", encoding="utf-8") as input_file:
        rows = list(csv.DictReader(input_file))
    if not rows:
        raise ValueError(f"No measurements found in {path}")
    return rows


def median(rows: Iterable[dict[str, str]], column: str) -> float:
    return statistics.median(float(row[column]) for row in rows)


def summarize(rows: list[dict[str, str]]) -> list[dict[str, float | int | str]]:
    grouped: dict[tuple[str, int], list[dict[str, str]]] = defaultdict(list)
    for row in rows:
        grouped[(row["family"], int(row["requested_n"]))].append(row)

    summary: list[dict[str, float | int | str]] = []
    for (family, requested_n), measurements in sorted(grouped.items()):
        first = measurements[0]
        result: dict[str, float | int | str] = {
            "family": family,
            "requested_n": requested_n,
            "n": int(first["n"]),
            "m": int(first["m"]),
            "work_size": int(first["work_size"]),
            "sample_count": len(measurements),
        }

        numeric_columns = (
            "total_ns",
            "validation_ns",
            "dense_shortcut_overhead_ns",
            "dfs_preprocessing_ns",
            "state_initialization_ns",
            "decision_core_ns",
            "failure_factory_ns",
            "kuratowski_preparation_ns",
            "kuratowski_oriented_state_copy_ns",
            "kuratowski_orientation_ns",
            "kuratowski_context_initialization_ns",
            "kuratowski_minor_classification_ns",
            "kuratowski_classify_initial_ns",
            "kuratowski_classify_external_face_vertices_ns",
            "kuratowski_find_highest_xy_path_ns",
            "kuratowski_find_z_to_root_path_ns",
            "kuratowski_find_future_pertinent_below_xy_path_ns",
            "kuratowski_isolation_ns",
            "certificate_verification_ns",
            "embedding_recovery_ns",
            "accounted_ns",
            "unaccounted_ns",
            "ns_per_work_item",
        )
        for column in numeric_columns:
            result[column] = median(measurements, column)

        result["preparation_remainder_ns"] = max(
            0.0,
            float(result["kuratowski_preparation_ns"])
            - float(result["kuratowski_oriented_state_copy_ns"])
            - float(result["kuratowski_orientation_ns"])
            - float(result["kuratowski_context_initialization_ns"])
            - float(result["kuratowski_minor_classification_ns"]),
        )
        result["classification_remainder_ns"] = max(
            0.0,
            float(result["kuratowski_minor_classification_ns"])
            - float(result["kuratowski_classify_initial_ns"])
            - float(result["kuratowski_classify_external_face_vertices_ns"])
            - float(result["kuratowski_find_highest_xy_path_ns"])
            - float(result["kuratowski_find_z_to_root_path_ns"])
            - float(result["kuratowski_find_future_pertinent_below_xy_path_ns"]),
        )
        summary.append(result)

    return summary


def save_summary(path: Path, rows: list[dict[str, float | int | str]]) -> None:
    with path.open("w", newline="", encoding="utf-8") as output_file:
        writer = csv.DictWriter(output_file, fieldnames=list(rows[0]))
        writer.writeheader()
        writer.writerows(rows)


def save_figure(fig: plt.Figure, output_base: Path) -> None:
    output_base.parent.mkdir(parents=True, exist_ok=True)
    fig.set_size_inches(
        PICTURE_WIDTH_CM * CM_TO_INCH,
        PICTURE_WIDTH_CM * HEIGHT_WIDTH_RATIO * CM_TO_INCH,
    )
    for suffix in (".png", ".pdf", ".svg"):
        fig.savefig(output_base.with_suffix(suffix), bbox_inches="tight")
    plt.close(fig)


def by_family(rows: list[dict[str, float | int | str]]) -> dict[str, list[dict[str, float | int | str]]]:
    grouped: dict[str, list[dict[str, float | int | str]]] = defaultdict(list)
    for row in rows:
        grouped[str(row["family"])].append(row)
    for values in grouped.values():
        values.sort(key=lambda row: int(row["n"]))
    return dict(sorted(grouped.items()))


def family_label(family: str) -> str:
    return {
        "subdivided_k33": r"Potpodjela $K_{3,3}$",
        "subdivided_k5": r"Potpodjela $K_5$",
    }.get(family, family)


def plot_total(rows: list[dict[str, float | int | str]], output_dir: Path) -> None:
    fig, ax = plt.subplots()
    for family, values in by_family(rows).items():
        ax.plot(
            [int(row["work_size"]) for row in values],
            [float(row["total_ns"]) / 1_000_000.0 for row in values],
            marker="o",
            label=family_label(family),
        )
    ax.set_xscale("log")
    ax.set_yscale("log")
    ax.set_xlabel(r"Veličina ulaza $n + m$")
    ax.set_ylabel("Medijana ukupnog vremena (ms)")
    ax.legend()
    save_figure(fig, output_dir / "phase_total_runtime")


def plot_normalized(rows: list[dict[str, float | int | str]], output_dir: Path) -> None:
    fig, ax = plt.subplots()
    for family, values in by_family(rows).items():
        ax.plot(
            [int(row["work_size"]) for row in values],
            [float(row["ns_per_work_item"]) for row in values],
            marker="o",
            label=family_label(family),
        )
    ax.set_xscale("log")
    ax.set_xlabel(r"Veličina ulaza $n + m$")
    ax.set_ylabel(r"Ukupno vrijeme po elementu ulaza (ns / $(n + m)$)")
    ax.legend()
    save_figure(fig, output_dir / "phase_normalized_total_runtime")


def plot_breakdown(
    rows: list[dict[str, float | int | str]],
    family: str,
    phases: tuple[tuple[str, str], ...],
    output_name: str,
    ylabel: str,
    output_dir: Path,
) -> None:
    values = [row for row in rows if row["family"] == family]
    values.sort(key=lambda row: int(row["n"]))
    if not values:
        return

    fig, ax = plt.subplots()
    for column, label in phases:
        x = [int(row["n"]) for row in values]
        y = [max(float(row[column]) / 1_000_000.0, 1e-9) for row in values]
        ax.plot(x, y, marker="o", label=label)
    ax.set_xscale("log")
    ax.set_yscale("log")
    ax.set_xlabel(r"Broj čvorova $n$")
    ax.set_ylabel(ylabel)
    ax.legend()
    save_figure(fig, output_dir / output_name)


def plot_preparation_share(rows: list[dict[str, float | int | str]], output_dir: Path) -> None:
    values = [row for row in rows if row["family"] == "subdivided_k5"]
    values.sort(key=lambda row: int(row["n"]))
    if not values:
        return

    x = [int(row["n"]) for row in values]
    series: list[list[float]] = []
    labels: list[str] = []
    for column, label in PREPARATION_PHASES:
        series.append(
            [
                100.0 * float(row[column]) / max(float(row["kuratowski_preparation_ns"]), 1.0)
                for row in values
            ]
        )
        labels.append(label)

    fig, ax = plt.subplots()
    ax.stackplot(x, *series, labels=labels, alpha=0.82)
    ax.set_xscale("log")
    ax.set_ylim(0.0, 100.0)
    ax.set_xlabel(r"Broj čvorova $n$")
    ax.set_ylabel("Udio vremena pripreme izolacije (%)")
    ax.legend(loc="center left", bbox_to_anchor=(1.02, 0.5))
    save_figure(fig, output_dir / "phase_k5_preparation_share")


def write_report(path: Path) -> None:
    path.write_text(
        """# Phase-level BM diagnostic figures

Generated files:

- `phase_summary.csv`: median timings for every phase and input size.
- `phase_total_runtime.*`: total runtime comparison for subdivided K3,3 and K5 inputs.
- `phase_normalized_total_runtime.*`: total normalized runtime.
- `phase_breakdown_subdivided_k33.*`: top-level phase breakdown for K3,3 subdivisions.
- `phase_breakdown_subdivided_k5.*`: top-level phase breakdown for K5 subdivisions.
- `phase_k5_preparation_breakdown.*`: detailed breakdown of Kuratowski-isolation preparation for K5 subdivisions.
- `phase_k5_preparation_share.*`: relative share of preparation subphases for K5 subdivisions.
- `phase_k5_classification_breakdown.*`: detailed classifier breakdown for K5 subdivisions.

Each figure is exported as PNG, PDF and SVG.
""",
        encoding="utf-8",
    )


def main() -> int:
    args = parse_args()
    configure_matplotlib()
    args.output_dir.mkdir(parents=True, exist_ok=True)

    rows = summarize(load_rows(args.input))
    save_summary(args.output_dir / "phase_summary.csv", rows)
    plot_total(rows, args.output_dir)
    plot_normalized(rows, args.output_dir)
    plot_breakdown(
        rows,
        "subdivided_k33",
        TOP_LEVEL_PHASES,
        "phase_breakdown_subdivided_k33",
        "Medijana vremena faze (ms)",
        args.output_dir,
    )
    plot_breakdown(
        rows,
        "subdivided_k5",
        TOP_LEVEL_PHASES,
        "phase_breakdown_subdivided_k5",
        "Medijana vremena faze (ms)",
        args.output_dir,
    )
    plot_breakdown(
        rows,
        "subdivided_k5",
        PREPARATION_PHASES,
        "phase_k5_preparation_breakdown",
        "Medijana vremena podfaze (ms)",
        args.output_dir,
    )
    plot_preparation_share(rows, args.output_dir)
    plot_breakdown(
        rows,
        "subdivided_k5",
        CLASSIFICATION_PHASES,
        "phase_k5_classification_breakdown",
        "Medijana vremena podfaze (ms)",
        args.output_dir,
    )
    write_report(args.output_dir / "README.md")

    print(f"Generated phase-diagnostic figures and tables in: {args.output_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
